/**
 * Captures up-to-date screenshots of the main Vuesp screens.
 *
 * Usage (from the `vue` directory):
 *
 *   npm run screen                        # all screens, dark theme, English
 *   npm run screen -- dashboard gpio      # only selected screens
 *   npm run screen -- --theme=light       # light theme
 *   npm run screen -- --width=1440 --height=900 --scale=2
 *   npm run screen -- --base-url=http://192.168.1.100   # real device
 *
 * By default it starts the mock device and the Vite dev server, so no hardware
 * is required. Screenshots are written to `../github/images/screen`.
 */
import { mkdir, stat } from 'node:fs/promises';
import { join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

import { chromium } from 'playwright';
import { createServer as createViteServer } from 'vite';

import { startMockDevice } from './mock-device.mjs';

// The tool lives in the core Vuesp repository, but the same script captures
// screenshots for device repositories. `VUESP_SCREEN_ROOT` points at the
// target `vue/` directory; without it, the script uses its own repository.
const ROOT = process.env.VUESP_SCREEN_ROOT
  ? resolve(process.env.VUESP_SCREEN_ROOT)
  : fileURLToPath(new URL('../..', import.meta.url));

const PAGES = [
  { name: 'home', path: '/', file: 'home.png' },
  { name: 'dashboard', path: '/dashboard/', file: 'dashboard.png' },
  { name: 'script', path: '/script/', file: 'script.png' },
  { name: 'settings', path: '/device/settings/', file: 'settings.png' },
  { name: 'storage', path: '/device/storage/', file: 'storage.png' },
  { name: 'gpio', path: '/device/gpio/', file: 'gpio.png' },
];

const HELP = `Vuesp screenshot tool

Usage:
  npm run screen [screens...] [options]

Screens:
  ${PAGES.map((page) => page.name).join(', ')}
  (default: all)

Options:
  --theme=dark|light        UI theme (default: dark)
  --locale=en|ru            UI language (default: en)
  --width=1200              viewport width (default: 1200)
  --height=900              viewport height (default: 900)
  --scale=1                 device scale factor, 2 for retina (default: 1)
  --out=../github/images/screen
                            output directory, relative to the vue/ folder
  --base-url=http://host:port
                            use a running app/device instead of the built-in mock
  --help                    show this help
`;

const parseArgs = (argv) => {
  const options = {
    names: [],
    theme: 'dark',
    locale: 'en',
    width: 1200,
    height: 900,
    scale: 1,
    out: '../github/images/screen',
    baseUrl: '',
  };

  for (let index = 0; index < argv.length; index += 1) {
    const arg = argv[index];
    if (arg === '--help' || arg === '-h') {
      console.log(HELP);
      process.exit(0);
    }
    if (!arg.startsWith('--')) {
      options.names.push(arg);
      continue;
    }

    const [rawKey, rawValue] = arg.slice(2).split('=');
    const key = rawKey.replace(/-([a-z])/g, (_, letter) => letter.toUpperCase());
    const value = rawValue ?? argv[index + 1];

    if (rawValue === undefined) index += 1;
    if (['width', 'height', 'scale'].includes(key)) options[key] = Number(value);
    else options[key] = value;
  }

  if (!['dark', 'light'].includes(options.theme)) {
    throw new Error(`Unknown theme "${options.theme}" (expected "dark" or "light")`);
  }
  if (!Number.isFinite(options.width) || options.width < 320) throw new Error('Invalid --width');
  if (!Number.isFinite(options.height) || options.height < 320) throw new Error('Invalid --height');
  if (!Number.isFinite(options.scale) || options.scale <= 0) throw new Error('Invalid --scale');

  const selected = options.names.length ? PAGES.filter((page) => options.names.includes(page.name)) : PAGES;
  if (!selected.length) {
    throw new Error(`Unknown screen(s): ${options.names.join(', ')}. Available: ${PAGES.map((page) => page.name).join(', ')}`);
  }

  return { ...options, pages: selected };
};

const waitForScreen = async (page, name) => {
  await page.waitForSelector('main h1', { timeout: 20_000 });
  await page.evaluate(() => document.fonts?.ready);

  // Data arrives over the WebSocket right after the first render.
  if (['dashboard', 'script', 'settings', 'storage', 'gpio'].includes(name)) {
    await page.waitForTimeout(900);
  } else {
    await page.waitForTimeout(350);
  }

  const disconnected = await page.evaluate(() => /Disconnected|Отключено/.test(document.body.innerText));
  if (disconnected) {
    throw new Error('device is not connected (WebSocket did not respond)');
  }
};

const main = async () => {
  let options;
  try {
    options = parseArgs(process.argv.slice(2));
  } catch (error) {
    console.error(`\n${error.message}\n`);
    console.log(HELP);
    process.exit(1);
  }

  const outDir = resolve(ROOT, options.out);
  await mkdir(outDir, { recursive: true });

  let mock = null;
  let vite = null;
  let browser = null;
  let baseUrl = options.baseUrl;

  const close = async () => {
    if (browser) await browser.close().catch(() => {});
    if (vite) await vite.close().catch(() => {});
    if (mock) await mock.close().catch(() => {});
  };

  process.once('SIGINT', async () => {
    await close();
    process.exit(130);
  });

  try {
    if (!baseUrl) {
      mock = await startMockDevice({ port: 4173, root: ROOT });
      vite = await createViteServer({
        root: ROOT,
        mode: 'screen',
        logLevel: 'warn',
        server: {
          host: '127.0.0.1',
          port: 3000,
          strictPort: true,
        },
      });
      await vite.listen();
      baseUrl = `http://127.0.0.1:3000`;
      console.log(`Vite dev server: ${baseUrl}`);
    } else {
      console.log(`Using existing app: ${baseUrl}`);
    }

    baseUrl = baseUrl.replace(/\/+$/, '');

    browser = await chromium.launch();
    const context = await browser.newContext({
      viewport: { width: options.width, height: options.height },
      deviceScaleFactor: options.scale,
      colorScheme: options.theme,
      locale: options.locale === 'ru' ? 'ru-RU' : 'en-US',
    });

    await context.addInitScript(
      ({ theme, language }) => {
        localStorage.setItem('theme', theme);
        localStorage.setItem('locale', language);
        // Suppress the "new version" information dialog on the first visit.
        localStorage.setItem('isShow', 'true');
        document.documentElement?.classList.remove('light', 'dark');
        document.documentElement?.classList.add(theme);
      },
      { theme: options.theme, language: options.locale }
    );

    const failed = [];

    for (const pageInfo of options.pages) {
      const page = await context.newPage();
      const url = `${baseUrl}${pageInfo.path}`;
      const target = join(outDir, pageInfo.file);

      try {
        console.log(`→ ${pageInfo.name} (${url})`);
        await page.goto(url, { waitUntil: 'networkidle', timeout: 30_000 });
        await page.addStyleTag({
          content: '*,*::before,*::after{animation-duration:0s!important;animation-delay:0s!important;transition-duration:0s!important;caret-color:transparent!important}',
        });
        await waitForScreen(page, pageInfo.name);
        await page.screenshot({ path: target, type: 'png' });

        const { size } = await stat(target);
        console.log(`  saved ${pageInfo.file} (${(size / 1024).toFixed(1)} KB)`);
      } catch (error) {
        failed.push(pageInfo.name);
        console.error(`  failed: ${error.message}`);
      } finally {
        await page.close();
      }
    }

    console.log(`\nScreenshots: ${outDir}`);
    if (failed.length) {
      console.error(`Failed screens: ${failed.join(', ')}`);
      process.exitCode = 1;
    }
  } finally {
    await close();
  }
};

await main();
