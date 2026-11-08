import { promises as fs } from 'fs';

const path = '../platformio.ini';

(async () => {
    try {
        let data = await fs.readFile(path, 'utf8');
        const version = data.match(/firmware_version = '{((.)*)}'/)?.[1]; // firmware_version = '{ 2, 0, 1 }'
        const [a, b, c] = version.split(',')
        const newVersion = `${+a}, ${+b}, ${+(c) + 1}`
        data = data.replace(/firmware_version = (.)*'/, `firmware_version = '{${newVersion}}'`);
        // console.log(data);

        await fs.writeFile(path, data);
    } catch (error) {
        console.log(error);
    }
})();