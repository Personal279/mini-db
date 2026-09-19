const { app, BrowserWindow } = require("electron");
const { spawn } = require("child_process");
const path = require("path");
const fs = require("fs");

let backendProcess;
let mainWindow;

function getBackendPath() {
    if (app.isPackaged) {
        return path.join(
            process.resourcesPath,
            "backend",
            "mini_couchbase.exe"
        );
    }

    return path.resolve(
        __dirname,
        "..",
        "..",
        "..",
        "build",
        "Release",
        "mini_couchbase.exe"
    );
}

function getBackendWorkingDirectory() {
    if (app.isPackaged) {
        return process.resourcesPath;
    }

    return path.resolve(
        __dirname,
        "..",
        "..",
        ".."
    );
}

function startBackend() {
    const backendPath = getBackendPath();
    const workingDirectory = getBackendWorkingDirectory();

    console.log("Backend:", backendPath);
    console.log("Working directory:", workingDirectory);

    if (!fs.existsSync(backendPath)) {
        console.error("Backend executable not found:", backendPath);
        return;
    }

    backendProcess = spawn(backendPath, [], {
        cwd: workingDirectory,
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
    const uiPath = path.resolve(
        __dirname,
        "..",
        "dist",
        "index.html"
    );

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
        backendProcess = null;
    }

    if (process.platform !== "darwin") {
        app.quit();
    }
});

app.on("before-quit", () => {
    if (backendProcess) {
        backendProcess.kill();
        backendProcess = null;
    }
});