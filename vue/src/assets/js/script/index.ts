export const examples = [
  { name: 'variable', value: "$v0=0;$v0=$v0+5;$v9=4561213;$i0=-1000;$s0='text:';$s0=$s0+$v0;" },
  { name: 'if', value: 'if:$v0==0;$v0=1;else;$v0=0;end;log($v0)' },
  { name: 'log()', value: "$v0=0;$v0=$v0+5;log($v0);$v9=4561213;$i0=-1000;log($v9,$i0);$s0='text:';$s0=$s0+$v0;$a0={1,2,3,4,5};log($s0, $a0)" },
  { name: 'wait()', value: "log('start');wait(1s);log('stop');wait(1s);" },
  { name: 'blink', value: '$v0=0;while:$v0<3;$p14=255;wait(1s);$p14=0;wait(1s);$v0=$v0+1;end;' },
  { name: 'event()', value: "on('btn_4');$p13=255;wait(1s);$p13=0;end;" },
  { name: 'while', value: '$v0=1;while:$v0>0;$p14=255;wait(1s);$p14=0;wait(1s);end;' },
];

export const enum ScriptType {
  SCRIPT_START = 10,
  SCRIPT_STOP = 11,
  SCRIPT_STOP_ALL = 12,
  SCRIPT_REMOVE = 13,
  SCRIPT_GET_ALL_SLOT = 20,
}
