import { promises as fs } from 'fs';
import { join, normalize } from 'path';

import VuespStruct from 'vuesp-struct';

const parseStruct = VuespStruct.default.parseStruct;

const pathStruct = normalize(process.cwd() + '/' + '../src/const/struct.h');
const pathJSON = normalize(process.cwd() + '/json/');

const pathOutput = normalize(process.cwd() + '/public');

const mergeJsonFiles = async (path) => {
  try {
    const mergeDirectory = async (currentPath) => {
      const items = await fs.readdir(currentPath);
      const result = {};
      for (const item of items) {
        const itemPath = join(currentPath, item);
        const stat = await fs.stat(itemPath);
        if (stat.isDirectory()) {
          result[item] = await mergeDirectory(itemPath);
        } else if (stat.isFile() && item.endsWith('.json')) {
          const fileContent = await fs.readFile(itemPath, 'utf8');
          const jsonData = JSON.parse(fileContent);
          const fileName = item.replace('.json', '');
          result[fileName] = jsonData;
        }
      }
      return result;
    };

    return await mergeDirectory(path);
  } catch (error) {
    console.error('Ошибка при объединении файлов:', error);
    throw error;
  }
};

(async () => {
  try {
    const dataJSON = await mergeJsonFiles(pathJSON);
    const dataStruct = await fs.readFile(pathStruct, 'utf8');

    const data = { ...dataJSON, struct: parseStruct(dataStruct) };
    const result = JSON.stringify(data);

    await fs.writeFile(`${pathOutput}/default.json`, result);
    console.log(`Save json: ${pathOutput} to default.json`);
  } catch (error) {
    console.log(error);
  }
})();
