const { app, BrowserWindow } = require("electron");
const { spawn } = require("child_process");
const path = require("path");

let backendProcess;
let mainWindow;

function startBackend() {
    const projectRoot = path.resolve(__dirname, "..", "..", "..");

    const backendPath = path.join(
        projectRoot,
        "build",
        "Release",
        "mini_couchbase.exe"
    );

    console.log("Project root:", projectRoot);
    console.log("Starting backend:", backendPath);

    backendProcess = spawn(backendPath, [], {
        cwd: projectRoot,
        windowsHide: true
    });

    backendProcess.on("error", (error) => {
        console.error("Failed to start Mini-Couchbase:", error);
    });

    backendProcess.on("exit", (code) => {
        console.log("Mini-Couchbase exited with code:", code);
    });
}

function createWindow() {
    const uiPath = path.resolve(__dirname, "..", "dist", "index.html");

    console.log("Loading UI:", uiPath);

    mainWindow = new BrowserWindow({
        width: 1400,
        height: 900,
        minWidth: 1000,
        minHeight: 700,
        title: "Mini-Couchbase",
        webPreferences: {
            contextIsolation: true,
            nodeIntegration: false
        }
    });

    mainWindow.loadFile(uiPath);
}

app.whenReady().then(() => {
    startBackend();

    setTimeout(() => {
        createWindow();
    }, 2000);
});

app.on("window-all-closed", () => {
    if (backendProcess) {
        backendProcess.kill();
    }

    if (process.platform !== "darwin") {
        app.quit();
    }
});

app.on("before-quit", () => {
    if (backendProcess) {
        backendProcess.kill();
    }
});