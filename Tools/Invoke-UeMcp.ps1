param(
    [Parameter(Mandatory = $true)]
    [string]$Method,

    [string]$ParamsJson = "{}",

    [string]$Url = "ws://127.0.0.1:9877",

    [int]$TimeoutSeconds = 30
)

$ErrorActionPreference = "Stop"

$client = [System.Net.WebSockets.ClientWebSocket]::new()
$cts = [System.Threading.CancellationTokenSource]::new([TimeSpan]::FromSeconds($TimeoutSeconds))

try {
    $null = $client.ConnectAsync([Uri]$Url, $cts.Token).GetAwaiter().GetResult()

    $payload = @{
        jsonrpc = "2.0"
        id = 1
        method = $Method
        params = $null
    }
    $payload.params = $ParamsJson | ConvertFrom-Json
    $json = $payload | ConvertTo-Json -Depth 64 -Compress
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($json)

    $null = $client.SendAsync(
        [ArraySegment[byte]]::new($bytes),
        [System.Net.WebSockets.WebSocketMessageType]::Text,
        $true,
        $cts.Token
    ).GetAwaiter().GetResult()

    $buffer = [byte[]]::new(1048576)
    $segment = [ArraySegment[byte]]::new($buffer)
    $builder = [System.Text.StringBuilder]::new()

    do {
        $result = $client.ReceiveAsync($segment, $cts.Token).GetAwaiter().GetResult()
        if ($result.Count -gt 0) {
            [void]$builder.Append([System.Text.Encoding]::UTF8.GetString($buffer, 0, $result.Count))
        }
    } while (-not $result.EndOfMessage)

    $client.Abort()
    $builder.ToString()
}
finally {
    if ($client) {
        $client.Dispose()
    }
    if ($cts) {
        $cts.Dispose()
    }
}
