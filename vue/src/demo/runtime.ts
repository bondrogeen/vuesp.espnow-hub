import VuespStructModule from 'vuesp-struct';

const VuespStruct = (VuespStructModule as unknown as { default?: typeof VuespStructModule }).default ?? VuespStructModule;

type MessageHandler = ((event: MessageEvent) => void) | null;
type EventHandler = ((event: Event) => void) | null;
type CloseHandler = ((event: CloseEvent) => void) | null;

const COMMAND = {
	GPIO_GET: 1,
	GPIO_SET: 2,
	GPIO_GET_ALL: 3,
	GPIO_CHANGE: 4,
	GPIO_SAVE: 254,
};

const LIST_ALL = 0xff;

const buildPort = (gpio: number, overrides: Record<string, unknown> = {}) => ({
	gpio,
	value: 0,
	mode: 1,
	interrupt: 0,
	list: LIST_ALL,
	disabled: 0,
	isButton: 0,
	command: 0,
	isPressed: 0,
	count: 0,
	empty: 0,
	valueOld: 0,
	pressStart: 0,
	...overrides,
});

const jsonResponse = (body: unknown): Response =>
	new Response(JSON.stringify(body), {
		status: 200,
		headers: { 'Content-Type': 'application/json; charset=utf-8' },
	});

const textResponse = (body: string, contentType = 'text/plain; charset=utf-8'): Response =>
	new Response(body, { status: 200, headers: { 'Content-Type': contentType } });


const installBadge = (firmwareVersion: string): void => {
	if (typeof document === 'undefined' || document.getElementById('vuesp-demo-badge')) return;

	const badge = document.createElement('div');
	badge.id = 'vuesp-demo-badge';
	badge.style.cssText = [
		'position:fixed',
		'right:16px',
		'bottom:16px',
		'z-index:2147483647',
		'display:flex',
		'align-items:center',
		'gap:10px',
		'padding:8px 10px',
		'border:1px solid rgba(255,255,255,.18)',
		'border-radius:8px',
		'background:rgba(6,11,40,.92)',
		'box-shadow:0 18px 50px -24px rgba(0,0,0,.9)',
		'backdrop-filter:blur(12px)',
		'color:#e2e8f0',
		'font:12px/1.2 Roboto,system-ui,sans-serif',
	].join(';');

	const text = document.createElement('span');
	text.textContent = `Демо · Vuesp ${firmwareVersion}`;

	const reset = document.createElement('button');
	reset.type = 'button';
	reset.textContent = 'Сбросить';
	reset.style.cssText = 'border:1px solid rgba(255,255,255,.18);border-radius:6px;background:rgba(255,255,255,.08);color:inherit;font:inherit;padding:4px 8px;cursor:pointer';
	reset.onclick = () => window.location.reload();

	badge.append(text, reset);
	document.body.appendChild(badge);
};

export const installDemoRuntime = (): void => {
	try {
		localStorage.setItem('isShow', 'true');
		if (!localStorage.getItem('theme')) localStorage.setItem('theme', 'dark');
		if (!localStorage.getItem('locale')) localStorage.setItem('locale', 'ru');
	} catch {
		// Private mode can block localStorage; the demo still works.
	}

	const data = __DEMO_DATA__;
	const defaultJson = JSON.parse(data.defaultJson);
	const dashboardJson = data.dashboardJson;
	const scriptsTxt = data.scriptsTxt;
	const firmwareVersion = data.firmwareVersion;
	const versionLabel = firmwareVersion.join('.');

	class DemoWebSocket {
		static readonly CONNECTING = 0;
		static readonly OPEN = 1;
		static readonly CLOSING = 2;
		static readonly CLOSED = 3;

		readonly CONNECTING = 0;
		readonly OPEN = 1;
		readonly CLOSING = 2;
		readonly CLOSED = 3;

		readyState = DemoWebSocket.CONNECTING;
		binaryType: BinaryType = 'arraybuffer';
		url: string;
		onopen: EventHandler = null;
		onmessage: MessageHandler = null;
		onclose: CloseHandler = null;
		onerror: EventHandler = null;
		keepAlive: ReturnType<typeof setInterval> | null = null;

		constructor(url: string | URL) {
			this.url = String(url);
			setTimeout(() => {
				this.readyState = DemoWebSocket.OPEN;
				this.onopen?.(new Event('open'));
				startSession(this);
			}, 60);
		}

		send(data: string | ArrayBufferLike | Blob | ArrayBufferView): void {
			if (this.readyState !== DemoWebSocket.OPEN) return;
			const buffer = data instanceof ArrayBuffer ? new Uint8Array(data) : ArrayBuffer.isView(data) ? new Uint8Array(data.buffer, data.byteOffset, data.byteLength) : null;
			if (!buffer) return;
			handleFrame(this, buffer);
		}

		close(code = 1000, reason = ''): void {
			if (this.readyState === DemoWebSocket.CLOSED) return;
			if (this.keepAlive) clearInterval(this.keepAlive);
			this.keepAlive = null;
			this.readyState = DemoWebSocket.CLOSED;
			this.onclose?.(new CloseEvent('close', { code, reason, wasClean: true }));
		}

		addEventListener(type: string, listener: EventListenerOrEventListenerObject | null): void {
			if (!listener) return;
			const call = (event: Event) => (typeof listener === 'function' ? listener(event) : listener.handleEvent(event));
			if (type === 'open') this.onopen = call;
			if (type === 'message') this.onmessage = call as MessageHandler;
			if (type === 'close') this.onclose = call as CloseHandler;
			if (type === 'error') this.onerror = call;
		}

		removeEventListener(): void {
			// The firmware UI uses property handlers only.
		}
	}

	const struct = new VuespStruct();
	struct.init(defaultJson.struct);

	const now = Math.floor(Date.now() / 1000);
	const deviceFields: Array<{ name: string }> = defaultJson?.struct?.structs?.Device ?? [];
	const names = new Set(deviceFields.map((field) => field.name));

	const deviceState = {
		info: {
			firmware: firmwareVersion,
			totalBytes: 1024 * 1024,
			usedBytes: 214 * 1024,
			id: 0xda129b,
			uptime: 4 * 3600 + 45 * 60 + 19,
			name: 'VUESP-DEMO',
			board: 1,
		},
		settings: {
			wifiDhcp: 1,
			wifiMode: 1,
			authMode: 0,
			version: 1,
			device: 0,
			discovery: 0,
			discoveryInterval: 5,
			discoveryPort: 0,
			wifiIp: [192, 168, 1, 100],
			wifiSubnet: [255, 255, 255, 0],
			wifiGateway: [192, 168, 1, 1],
			wifiDns: [192, 168, 1, 1],
			wifiSsid: 'Vuesp Demo',
			wifiPass: '',
			authLogin: 'admin',
			authPass: '',
		},
		device: {
			key: 0,
			command: 0,
			analog: 512,
			now,
			list: 2,
			message: 'text',
			...(names.has('pwm') ? { pwm: 128 } : {}),
		} as Record<string, unknown>,
		buffer: {
			empty: 0,
			head: 0,
			tail: 0,
			count: 128,
			data: Array.from({ length: 256 }, (_, index) => Math.round(24 + 4 * Math.sin(index / 9))),
		},
		dallas: [
			{ comm: 0, comm2: 0, address: [0x28, 0xaa, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66], temp: 23.4 },
			{ comm: 1, comm2: 0, address: [0x28, 0xbb, 0x11, 0x22, 0x33, 0x44, 0x55, 0x77], temp: 18.7 },
		],
		ports: new Map<number, Record<string, unknown>>([
			[2, buildPort(2, { mode: 2, value: 1, interrupt: 2, isButton: 1 })],
			[4, buildPort(4, { value: 1 })],
			[5, buildPort(5, { value: 1 })],
			[12, buildPort(12, { value: 0 })],
			[13, buildPort(13, { value: 0 })],
			[14, buildPort(14, { mode: 128, value: 128 })],
			[15, buildPort(15, { mode: 0, value: 1, interrupt: 3, isButton: 1 })],
			[16, buildPort(16, { mode: 130, value: 512 })],
			[17, buildPort(17, { mode: 130, value: 23 })],
			[18, buildPort(18, { mode: 131, value: 128 })],
			[19, buildPort(19, { mode: 129, value: 0 })],
		]),
		files: {
			'/': [
				{ isDir: 1, isFile: 0, size: 0, name: 'www' },
				{ isDir: 1, isFile: 0, size: 0, name: 'tmp' },
				{ isDir: 1, isFile: 0, size: 0, name: 'docs' },
				{ isDir: 0, isFile: 1, size: 2048, name: 'config.json' },
				{ isDir: 0, isFile: 1, size: 65536, name: 'scripts.txt' },
			],
			'/www': [
				{ isDir: 1, isFile: 0, size: 0, name: 'assets' },
				{ isDir: 0, isFile: 1, size: 1352, name: 'index.html' },
				{ isDir: 0, isFile: 1, size: 5383, name: 'default.json' },
				{ isDir: 0, isFile: 1, size: 65536, name: 'scripts.txt' },
				{ isDir: 0, isFile: 1, size: 1150, name: 'favicon.ico' },
			],
			'/www/assets': [
				{ isDir: 0, isFile: 1, size: 356770, name: 'index.js' },
				{ isDir: 0, isFile: 1, size: 76510, name: 'style.css' },
				{ isDir: 0, isFile: 1, size: 12183, name: 'vendor.js' },
			],
			'/tmp': [
				{ isDir: 1, isFile: 0, size: 0, name: 'logs' },
				{ isDir: 0, isFile: 1, size: 2196, name: 'dashboard.json' },
				{ isDir: 0, isFile: 1, size: 128, name: 'gpio.io' },
			],
			'/tmp/logs': [
				{ isDir: 0, isFile: 1, size: 4096, name: 'boot.log' },
				{ isDir: 0, isFile: 1, size: 8192, name: 'script.log' },
			],
			'/docs': [
				{ isDir: 1, isFile: 0, size: 0, name: 'scriptrunner' },
				{ isDir: 0, isFile: 1, size: 3072, name: 'README.md' },
				{ isDir: 0, isFile: 1, size: 6144, name: 'api.md' },
			],
			'/docs/scriptrunner': [
				{ isDir: 0, isFile: 1, size: 9216, name: 'ru.md' },
				{ isDir: 0, isFile: 1, size: 8704, name: 'en.md' },
			],
		} as Record<string, Array<Record<string, unknown>>>,
		scanList: [
			{ id: 0, isHidden: 0, encryptionType: 4, channel: 1, rssi: -42, ssid: 'Vuesp Demo' },
			{ id: 1, isHidden: 0, encryptionType: 4, channel: 6, rssi: -61, ssid: 'Home-2.4G' },
			{ id: 2, isHidden: 1, encryptionType: 0, channel: 11, rssi: -78, ssid: '' },
		],
	};

	const encode = (key: string, payload?: Record<string, unknown>) => {
		const buffer = payload ? struct.set(key, payload as never) : struct.set(key);
		return buffer ? new Uint8Array(buffer as ArrayBuffer) : null;
	};

	const send = (socket: DemoWebSocket, key: string, payload?: Record<string, unknown>) => {
		const bytes = encode(key, payload);
		if (bytes && socket.readyState === DemoWebSocket.OPEN) {
			socket.onmessage?.(new MessageEvent('message', { data: bytes.buffer }));
		}
	};

	const sendPorts = (socket: DemoWebSocket, ports: Array<Record<string, unknown>>) => {
		for (const port of ports) send(socket, 'PORT', port);
	};

	const startSession = (socket: DemoWebSocket) => {
		send(socket, 'INFO', deviceState.info);
		send(socket, 'SETTINGS', deviceState.settings);
		deviceState.device.now = Math.floor(Date.now() / 1000);
		send(socket, 'DEVICE', deviceState.device);
		send(socket, 'BUFFER', deviceState.buffer);
		sendPorts(socket, [...deviceState.ports.values()]);
		for (const sensor of deviceState.dallas) send(socket, 'DALLAS', sensor);
		if (socket.keepAlive) clearInterval(socket.keepAlive);
		socket.keepAlive = setInterval(() => send(socket, 'PING'), 2000);
	};

	const handleFrame = (socket: DemoWebSocket, bytes: Uint8Array) => {
		const message = struct.get(bytes.buffer as ArrayBuffer) as { key?: string; object?: Record<string, unknown> } | null;
		if (!message?.key) return;

		switch (message.key) {
			case 'INFO':
				send(socket, 'INFO', deviceState.info);
				break;
			case 'SETTINGS':
				send(socket, 'SETTINGS', deviceState.settings);
				break;
			case 'DEVICE':
				deviceState.device.now = Math.floor(Date.now() / 1000);
				send(socket, 'DEVICE', deviceState.device);
				break;
			case 'PORT': {
				const object = (message.object ?? {}) as { command?: number; gpio?: number; value?: number; mode?: number; interrupt?: number; isButton?: number };
				const command = object.command ?? 0;
				if (command === COMMAND.GPIO_GET_ALL) {
					sendPorts(socket, [...deviceState.ports.values()]);
					break;
				}
				const port = deviceState.ports.get(object.gpio ?? -1);
				if (!port) break;
				if (command === COMMAND.GPIO_SET || command === COMMAND.GPIO_CHANGE) {
					Object.assign(port, {
						value: object.value ?? port.value,
						mode: object.mode ?? port.mode,
						interrupt: object.interrupt ?? port.interrupt,
						isButton: object.isButton ?? port.isButton,
					});
				}
				if (command !== COMMAND.GPIO_SAVE) send(socket, 'PORT', port);
				break;
			}
			case 'BUFFER':
				send(socket, 'BUFFER', deviceState.buffer);
				break;
			case 'DALLAS':
				for (const sensor of deviceState.dallas) send(socket, 'DALLAS', sensor);
				break;
			case 'DISCOVERY':
				break;
			case 'FILES': {
				const object = (message.object ?? {}) as { command?: number; name?: string };
				if ((object.command ?? 0) !== 0) break;
				const request = object.name ?? '/';
				const key = request !== '/' && request.endsWith('/') ? request.slice(0, -1) : request;
				for (const file of deviceState.files[key] ?? []) send(socket, 'FILES', { command: 0, ...file });
				break;
			}
			case 'SCAN':
				for (const network of deviceState.scanList) send(socket, 'SCAN', network);
				break;
			case 'REBOOT':
				break;
			case 'PING':
				send(socket, 'PING');
				break;
			default:
				break;
		}
	};

	const nativeFetch = window.fetch.bind(window);
	window.fetch = async (input: RequestInfo | URL, init?: RequestInit): Promise<Response> => {
		const raw = typeof input === 'string' ? input : input instanceof URL ? input.toString() : input.url;
		const url = new URL(raw, window.location.href);
		const path = url.pathname;

		if (path.endsWith('/default.json')) return textResponse(data.defaultJson, 'application/json; charset=utf-8');
		if (path.endsWith('/fs')) {
			if ((init?.method ?? 'GET').toUpperCase() !== 'GET') return jsonResponse({ state: 1 });
			const file = url.searchParams.get('file') ?? '';
			if (file === '/tmp/dashboard.json') return textResponse(dashboardJson, 'application/json; charset=utf-8');
			if (file === '/www/scripts.txt') return textResponse(scriptsTxt);
			if (file === '/tmp/gpio.io') return textResponse('');
			return new Response('Not found', { status: 404 });
		}
		if (path.endsWith('/cmd') || path.endsWith('/update')) return jsonResponse({ state: 1 });
		return nativeFetch(input, init);
	};

	const NativeWebSocket = window.WebSocket;
	window.WebSocket = new Proxy(NativeWebSocket, {
		construct(target, args) {
			if (String(args[0]).includes('/esp')) return new DemoWebSocket(args[0] as string | URL);
			return Reflect.construct(target, args);
		},
	}) as typeof WebSocket;

	installBadge(versionLabel);
	(window as unknown as { __VUESP_DEMO__?: boolean }).__VUESP_DEMO__ = true;
};

export {};
