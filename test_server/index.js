const http = require("http");
const fs = require("fs");

const server = http.createServer((req, res) => {
  const path =
    req.url === "/index.js" || req.url === "/index.wasm"
      ? "../dist" + req.url
      : "." + req.url;

  const contents = fs.readFileSync(path);

  res.write(contents, () => {
    res.statusCode = 200;
    res.end();
  });
});

server.listen(2956, () => console.log("http://localhost:2956/index.html"));
