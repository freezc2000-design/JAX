// 直连 UE_MCP_Bridge 设 DA_AuraInputAction.AbilityInputActions(真数组,8条,含 InputTag.7)
const IA = '/Game/Blueprints/Input/InputActions';
const ent = (a, t) => ({ InputAction: `${IA}/${a}.${a}`, InputTag: `(TagName="${t}")` });
const value = [
  ent('IA_LMB', 'InputTag.LMB'),
  ent('IA_KeyNum1', 'InputTag.1'),
  ent('IA_KeyNum2', 'InputTag.2'),
  ent('IA_KeyNum3', 'InputTag.3'),
  ent('IA_KeyNum4', 'InputTag.4'),
  ent('IA_KeyNum5', 'InputTag.5'),
  ent('IA_KeyNum6', 'InputTag.6'),
  ent('IA_KeyNum7', 'InputTag.7'),
];
const msg = {
  method: 'set_property', id: 1,
  params: {
    objectPath: '/Game/Blueprints/Input/DA_AuraInputAction.DA_AuraInputAction',
    propertyName: 'AbilityInputActions',
    value,
  },
};
const ws = new WebSocket('ws://127.0.0.1:9877');
ws.onopen = () => ws.send(JSON.stringify(msg));
ws.onmessage = (e) => { console.log('RESP:', typeof e.data === 'string' ? e.data : e.data.toString()); ws.close(); process.exit(0); };
ws.onerror = (e) => { console.log('ERR:', e.message || e); process.exit(1); };
setTimeout(() => { console.log('TIMEOUT'); process.exit(1); }, 12000);
