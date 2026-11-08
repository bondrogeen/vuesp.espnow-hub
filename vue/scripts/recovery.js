import { promises as fs } from 'fs';
import path from 'path'
import { gzip } from 'node-gzip'
import { minify } from 'html-minifier-terser'

const args = process.argv;
const inputFile = args[2];
const outputFile = args[3];
if (!inputFile) process.exit(1);

const pathInput = path.normalize(process.cwd() + '/' + inputFile);
const pathOutput = outputFile ? process.cwd() + '/' + outputFile : pathInput;

// var { base } = path.parse(pathOutputFile);

const options = {
  removeComments: true,
  removeCommentsFromCDATA: true,
  removeCDATASectionsFromCDATA: true,
  collapseWhitespace: true,
  collapseBooleanAttributes: true,
  removeAttributeQuotes: true,
  removeRedundantAttributes: true,
  useShortDoctype: true,
  removeEmptyAttributes: true,
  removeEmptyElements: false,
  removeOptionalTags: true,
  removeScriptTypeAttributes: true,
  removeStyleLinkTypeAttributes: true,
  minifyJS: true,
  minifyCSS: true,
};

const toHexString = byteArray => Array.from(byteArray, byte => '0x' + ('0' + (byte & 0xff).toString(16)).slice(-2)).join(',');

(async () => {
  try {
    const files = await fs.readdir(pathInput);
    files.forEach(async file => {
      var { base, name } = path.parse(`${pathInput}/${file}`);
      const data = await fs.readFile(`${pathInput}/${base}`, 'utf8');
      const result = await minify(data, options);
      const gz = await gzip(result);
      const text = `const uint8_t ${name}[] PROGMEM = { ${toHexString([...gz])} };`;
      await fs.writeFile(`${pathOutput}/${name}.h`, text);
      console.log(`Convert file: ${file} to ${name}.h`)
    });
  } catch (e) {
    console.log(e);
  }
})();
