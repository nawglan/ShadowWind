/*
  ShadowWind MUD WebSocket-to-Telnet Proxy
  Based on Maldorne mud-web-proxy (MIT license)
  Adapted for Kubernetes sidecar deployment
*/

import http from 'http';
import net from 'net';
import zlib from 'zlib';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { dirname } from 'path';
import { WebSocketServer } from 'ws';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const PORT = parseInt(process.env.PORT) || 8080;
const MUD_HOST = process.env.MUD_HOST || '127.0.0.1';
const MUD_PORT = parseInt(process.env.MUD_PORT) || 9999;
const BASE_PATH = process.env.BASE_PATH || '/legacy';
const DEBUG = process.env.DEBUG === 'true';

// Telnet protocol constants
const prt = {
  IAC: 255,
  SB: 250,
  SE: 240,
  WILL: 251,
  WONT: 252,
  DO: 253,
  DONT: 254,
  ECHO: 1,
  SGA: 3,
  TTYPE: 24,
  NAWS: 31,
  NEW: 39,
  CHARSET: 42,
  MSDP: 69,
  MCCP2: 86,
  MXP: 91,
  ATCP: 200,
  GMCP: 201,
  IS: 0,
  REQUEST: 1,
  MSDP_VAR: 1,
  MSDP_VAL: 2,

  // Pre-built responses
  WILL_TTYPE: Buffer.from([255, 251, 24]),
  WILL_GMCP: Buffer.from([255, 251, 201]),
  DO_GMCP: Buffer.from([255, 253, 201]),
  DO_MCCP: Buffer.from([255, 253, 86]),
  DO_MSDP: Buffer.from([255, 253, 69]),
  WILL_CHARSET: Buffer.from([255, 251, 42]),
  START_GMCP: Buffer.from([255, 250, 201]),
  STOP: Buffer.from([255, 240]),
};

function log(msg, socket) {
  const addr = socket?.remoteAddress || '';
  console.log(`${new Date().toISOString()} ${addr}: ${msg}`);
}

// HTTP server for serving the HTML client
const httpServer = http.createServer((req, res) => {
  let urlPath = req.url.split('?')[0];

  // Handle base path
  if (urlPath.startsWith(BASE_PATH)) {
    urlPath = urlPath.slice(BASE_PATH.length) || '/';
  }

  if (urlPath === '/' || urlPath === '/index.html') {
    res.writeHead(200, { 'Content-Type': 'text/html' });
    fs.createReadStream(path.join(__dirname, 'index.html')).pipe(res);
  } else {
    res.writeHead(404);
    res.end('Not Found');
  }
});

// WebSocket server
const wss = new WebSocketServer({
  server: httpServer,
  path: BASE_PATH + '/ws'
});

wss.on('connection', (ws, req) => {
  const remoteAddress = req.headers['x-forwarded-for'] || req.socket.remoteAddress;
  log('New WebSocket connection', { remoteAddress });

  // Session state
  const session = {
    remoteAddress,
    compressed: false,
    ttype: ['ShadowWind-WebClient', 'XTERM-256COLOR', 'MTTS 141', remoteAddress],
    echo_negotiated: false,
    gmcp_negotiated: false,
    msdp_negotiated: false,
    mxp_negotiated: false,
    ttype_negotiated: false,
    sga_negotiated: false,
    naws_negotiated: false,
    utf8_negotiated: false,
    new_negotiated: false,
    new_handshake: false,
    mccp_negotiated: false,
    password_mode: false,
  };

  // Connect to MUD
  const mudSocket = net.createConnection(MUD_PORT, MUD_HOST, () => {
    log(`Connected to MUD at ${MUD_HOST}:${MUD_PORT}`, { remoteAddress });

    // Set timeout for negotiations to complete
    setTimeout(() => {
      session.echo_negotiated = true;
      session.gmcp_negotiated = true;
      session.msdp_negotiated = true;
      session.mxp_negotiated = true;
      session.ttype_negotiated = true;
      session.sga_negotiated = true;
      session.naws_negotiated = true;
      session.utf8_negotiated = true;
      session.new_negotiated = true;
      session.mccp_negotiated = true;
    }, 12000);
  });

  // Helper to send data to client (base64 encoded, optionally compressed)
  function sendToClient(data) {
    if (ws.readyState !== 1) return; // WebSocket.OPEN

    // Compress data for client
    zlib.deflateRaw(data, (err, buffer) => {
      if (!err) {
        ws.send(buffer.toString('base64'));
      } else {
        // Fallback to uncompressed base64
        ws.send(data.toString('base64'));
      }
    });
  }

  // Helper functions for telnet protocol
  function sendTTYPE(msg) {
    if (msg) {
      mudSocket.write(prt.WILL_TTYPE);
      mudSocket.write(Buffer.from([prt.IAC, prt.SB, prt.TTYPE, prt.IS]));
      mudSocket.write(msg);
      mudSocket.write(Buffer.from([prt.IAC, prt.SE]));
      log(`TTYPE: ${msg}`, { remoteAddress });
    }
  }

  function sendGMCP(msg) {
    mudSocket.write(prt.START_GMCP);
    mudSocket.write(msg);
    mudSocket.write(prt.STOP);
  }

  function sendMSDPPair(key, val) {
    mudSocket.write(Buffer.from([prt.IAC, prt.SB, prt.MSDP, prt.MSDP_VAR]));
    mudSocket.write(key);
    mudSocket.write(Buffer.from([prt.MSDP_VAL]));
    mudSocket.write(val);
    mudSocket.write(Buffer.from([prt.IAC, prt.SE]));
    log(`MSDP: ${key}=${val}`, { remoteAddress });
  }

  // Process telnet protocol in MUD data
  function processAndSend(data) {
    // Handle MCCP compression start
    if (!session.mccp_negotiated && !session.compressed) {
      for (let i = 0; i < data.length; i++) {
        if (data[i] === prt.IAC && data[i + 1] === prt.WILL && data[i + 2] === prt.MCCP2) {
          setTimeout(() => {
            log('IAC DO MCCP2', { remoteAddress });
            mudSocket.write(prt.DO_MCCP);
          }, 1000);
        } else if (data[i] === prt.IAC && data[i + 1] === prt.SB && data[i + 2] === prt.MCCP2) {
          if (i) sendToClient(data.slice(0, i));
          data = data.slice(i + 5);
          session.compressed = true;
          log('MCCP compression started', { remoteAddress });
          if (!data.length) return;
        }
      }
    }

    // Handle TTYPE negotiation
    if (session.ttype.length) {
      for (let i = 0; i < data.length; i++) {
        if (data[i] === prt.IAC && data[i + 1] === prt.DO && data[i + 2] === prt.TTYPE) {
          log('IAC DO TTYPE', { remoteAddress });
          sendTTYPE(session.ttype.shift());
        } else if (data[i] === prt.IAC && data[i + 1] === prt.SB &&
                   data[i + 2] === prt.TTYPE && data[i + 3] === prt.REQUEST) {
          log('IAC SB TTYPE REQUEST', { remoteAddress });
          sendTTYPE(session.ttype.shift());
        }
      }
    }

    // Handle GMCP negotiation
    if (!session.gmcp_negotiated) {
      for (let i = 0; i < data.length; i++) {
        if (data[i] === prt.IAC &&
            (data[i + 1] === prt.DO || data[i + 1] === prt.WILL) &&
            data[i + 2] === prt.GMCP) {
          log('GMCP negotiation', { remoteAddress });
          if (data[i + 1] === prt.DO) {
            mudSocket.write(prt.WILL_GMCP);
          } else {
            mudSocket.write(prt.DO_GMCP);
          }
          session.gmcp_negotiated = true;
          sendGMCP('client ShadowWind-WebClient');
          sendGMCP('client_version 1.0');
          sendGMCP('client_ip ' + remoteAddress);
        }
      }
    }

    // Handle MSDP negotiation
    if (!session.msdp_negotiated) {
      for (let i = 0; i < data.length; i++) {
        if (data[i] === prt.IAC && data[i + 1] === prt.WILL && data[i + 2] === prt.MSDP) {
          mudSocket.write(prt.DO_MSDP);
          log('IAC WILL MSDP <- IAC DO MSDP', { remoteAddress });
          sendMSDPPair('CLIENT_ID', 'ShadowWind-WebClient');
          sendMSDPPair('CLIENT_VERSION', '1.0');
          sendMSDPPair('CLIENT_IP', remoteAddress);
          sendMSDPPair('XTERM_256_COLORS', '1');
          sendMSDPPair('UTF_8', '1');
          session.msdp_negotiated = true;
        }
      }
    }

    // Handle MXP negotiation
    if (!session.mxp_negotiated) {
      for (let i = 0; i < data.length; i++) {
        if (data[i] === prt.IAC && data[i + 1] === prt.DO && data[i + 2] === prt.MXP) {
          mudSocket.write(Buffer.from([prt.IAC, prt.WILL, prt.MXP]));
          log('IAC DO MXP <- IAC WILL MXP', { remoteAddress });
          session.mxp_negotiated = true;
        } else if (data[i] === prt.IAC && data[i + 1] === prt.WILL && data[i + 2] === prt.MXP) {
          mudSocket.write(Buffer.from([prt.IAC, prt.DO, prt.MXP]));
          log('IAC WILL MXP <- IAC DO MXP', { remoteAddress });
          session.mxp_negotiated = true;
        }
      }
    }

    // Handle NEW-ENV negotiation
    if (!session.new_negotiated) {
      for (let i = 0; i < data.length; i++) {
        if (data[i] === prt.IAC && data[i + 1] === prt.DO && data[i + 2] === prt.NEW) {
          mudSocket.write(Buffer.from([prt.IAC, prt.WILL, prt.NEW]));
          log('IAC WILL NEW-ENV', { remoteAddress });
          session.new_negotiated = true;
        }
      }
    } else if (!session.new_handshake) {
      for (let i = 0; i < data.length; i++) {
        if (data[i] === prt.IAC && data[i + 1] === prt.SB &&
            data[i + 2] === prt.NEW && data[i + 3] === prt.REQUEST) {
          mudSocket.write(Buffer.from([prt.IAC, prt.SB, prt.NEW, prt.IS, prt.IS]));
          mudSocket.write('IPADDRESS');
          mudSocket.write(Buffer.from([prt.REQUEST]));
          mudSocket.write(remoteAddress);
          mudSocket.write(Buffer.from([prt.IAC, prt.SE]));
          log('IAC NEW-ENV IP VAR SEND', { remoteAddress });
          session.new_handshake = true;
        }
      }
    }

    // Handle ECHO negotiation (for password hiding)
    for (let i = 0; i < data.length; i++) {
      if (data[i] === prt.IAC && data[i + 1] === prt.WILL && data[i + 2] === prt.ECHO) {
        log('IAC WILL ECHO (password mode - disable local echo)', { remoteAddress });
        session.password_mode = true;
        // Tell client to disable local echo
        if (ws.readyState === 1) {
          ws.send(JSON.stringify({ echo: false }));
        }
      } else if (data[i] === prt.IAC && data[i + 1] === prt.WONT && data[i + 2] === prt.ECHO) {
        log('IAC WONT ECHO (enable local echo)', { remoteAddress });
        session.password_mode = false;
        // Tell client to enable local echo
        if (ws.readyState === 1) {
          ws.send(JSON.stringify({ echo: true }));
        }
      }
    }

    // Handle SGA negotiation
    if (!session.sga_negotiated) {
      for (let i = 0; i < data.length; i++) {
        if (data[i] === prt.IAC && data[i + 1] === prt.WILL && data[i + 2] === prt.SGA) {
          mudSocket.write(Buffer.from([prt.IAC, prt.DO, prt.SGA]));
          log('IAC WILL SGA <- IAC DO SGA', { remoteAddress });
          session.sga_negotiated = true;
        }
      }
    }

    // Handle NAWS negotiation
    if (!session.naws_negotiated) {
      for (let i = 0; i < data.length; i++) {
        if (data[i] === prt.IAC && data[i + 1] === prt.DO && data[i + 2] === prt.NAWS) {
          mudSocket.write(Buffer.from([prt.IAC, prt.WONT, prt.NAWS]));
          log('IAC DO NAWS <- IAC WONT NAWS', { remoteAddress });
          session.naws_negotiated = true;
        }
      }
    }

    // Handle CHARSET/UTF-8 negotiation
    if (!session.utf8_negotiated) {
      for (let i = 0; i < data.length; i++) {
        if (data[i] === prt.IAC && data[i + 1] === prt.DO && data[i + 2] === prt.CHARSET) {
          mudSocket.write(prt.WILL_CHARSET);
          log('IAC DO CHARSET <- IAC WILL CHARSET', { remoteAddress });
        }
        if (data[i] === prt.IAC && data[i + 1] === prt.SB && data[i + 2] === prt.CHARSET) {
          mudSocket.write(Buffer.from([255, 250, 42, 2, 85, 84, 70, 45, 56, 255, 240]));
          log('UTF-8 negotiated', { remoteAddress });
          session.utf8_negotiated = true;
        }
      }
    }

    if (DEBUG) {
      const hex = data.toString('hex').substring(0, 100);
      log(`MUD -> Client: ${data.length} bytes: ${hex}`, { remoteAddress });
    }

    // Send data to client
    sendToClient(data);
  }

  // MUD socket events
  mudSocket.on('data', (data) => {
    if (DEBUG) {
      // Log raw bytes for debugging
      const bytes = [];
      for (let i = 0; i < Math.min(data.length, 50); i++) {
        bytes.push(data[i]);
      }
      log(`MUD raw data: [${bytes.join(',')}] (${data.length} bytes)`, { remoteAddress });
    }
    processAndSend(data);
  });

  mudSocket.on('close', (hadError) => {
    log(`MUD connection closed (hadError: ${hadError})`, { remoteAddress });
    if (ws.readyState === 1) {
      ws.close(1000, 'MUD disconnected');
    }
  });

  mudSocket.on('error', (err) => {
    log(`MUD socket error: ${err.message}`, { remoteAddress });
    if (ws.readyState === 1) {
      ws.close(1011, 'MUD connection error');
    }
  });

  // WebSocket events
  ws.on('message', (data) => {
    // Data from client - forward to MUD
    const buf = Buffer.isBuffer(data) ? data : Buffer.from(data);

    if (DEBUG) {
      const bytes = [];
      for (let i = 0; i < buf.length; i++) bytes.push(buf[i]);
      log(`Client -> MUD: [${bytes.join(',')}] "${buf.toString().replace(/\r/g, '\\r').replace(/\n/g, '\\n')}"`, { remoteAddress });
    }

    // Reset password mode after forwarding
    if (session.password_mode) {
      session.password_mode = false;
    }

    if (!mudSocket.destroyed) {
      mudSocket.write(buf);
    } else {
      log('Cannot write to MUD - socket destroyed', { remoteAddress });
    }
  });

  ws.on('close', () => {
    log('WebSocket closed', { remoteAddress });
    if (!mudSocket.destroyed) {
      mudSocket.destroy();
    }
  });

  ws.on('error', (err) => {
    log(`WebSocket error: ${err.message}`, { remoteAddress });
    if (!mudSocket.destroyed) {
      mudSocket.destroy();
    }
  });
});

httpServer.listen(PORT, () => {
  console.log(`ShadowWind WebClient listening on port ${PORT}`);
  console.log(`Base path: ${BASE_PATH}`);
  console.log(`MUD target: ${MUD_HOST}:${MUD_PORT}`);
});
