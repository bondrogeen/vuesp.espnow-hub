/**
 * Starts the mock device and the Vite dev server without a real ESP board.
 *
 *   npm run dev:mock
 *
 * The UI behaves like it is connected to a device: dashboard data, GPIO ports,
 * file system and settings are served by scripts/screen/mock-device.mjs.
 */
import { createServer as createViteServer } from 'vite';

import { startMockDevice } from './mock-device.mjs';

const mock = await startMockDevice({ port: 4173 });
const vite = await createViteServer({
  mode: 'screen',
  server: {
    host: '127.0.0.1',
    port: 3000,
    strictPort: true,
  },
});

await vite.listen();
console.log('Open http://127.0.0.1:3000');

const close = async () => {
  await vite.close();
  await mock.close();
  process.exit(0);
};

process.once('SIGINT', close);
process.once('SIGTERM', close);
