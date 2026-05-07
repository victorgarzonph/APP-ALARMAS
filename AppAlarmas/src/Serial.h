#pragma once
#include <windows.h>
#include <string>

class Serial {
private:
    HANDLE  handler;
    bool    connected;
    char    portName[16];

    void applySettings() {
        DCB params = { 0 };
        params.DCBlength = sizeof(params);
        GetCommState(handler, &params);
        params.BaudRate = CBR_115200;
        params.ByteSize = 8;
        params.StopBits = ONESTOPBIT;
        params.Parity   = NOPARITY;
        SetCommState(handler, &params);

        COMMTIMEOUTS t = { 0 };
        // Lectura: retorna inmediatamente si no hay bytes
        t.ReadIntervalTimeout         = MAXDWORD;
        t.ReadTotalTimeoutConstant    = 0;
        t.ReadTotalTimeoutMultiplier  = 0;
        // Escritura: maximo 300 ms — nunca bloquea el hilo de UI
        t.WriteTotalTimeoutConstant   = 300;
        t.WriteTotalTimeoutMultiplier = 0;
        SetCommTimeouts(handler, &t);
    }

    void tryOpen() {
        handler = CreateFileA(portName,
            GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);

        if (handler == INVALID_HANDLE_VALUE) {
            connected = false;
            return;
        }
        applySettings();
        connected = true;
    }

public:
    Serial(const char* port) : handler(INVALID_HANDLE_VALUE), connected(false) {
        strncpy_s(portName, sizeof(portName), port, _TRUNCATE);
        tryOpen();
    }

    // Detecta desconexion fisica real cada vez que se llama
    bool isConnected() {
        if (handler == INVALID_HANDLE_VALUE) {
            tryOpen();          // intenta reconectar automaticamente
            return connected;
        }
        COMSTAT cs; DWORD err;
        if (!ClearCommError(handler, &err, &cs)) {
            CloseHandle(handler);
            handler    = INVALID_HANDLE_VALUE;
            connected  = false;
            tryOpen();          // intenta reconectar automaticamente
        }
        return connected;
    }

    bool available() {
        if (!connected || handler == INVALID_HANDLE_VALUE) return false;
        COMSTAT cs; DWORD err;
        if (!ClearCommError(handler, &err, &cs)) {
            connected = false;
            return false;
        }
        return cs.cbInQue > 0;
    }

    std::string readLine() {
        if (!connected || handler == INVALID_HANDLE_VALUE) return "";

        // Leer solo los bytes que YA están en el buffer — nunca esperar
        COMSTAT cs; DWORD err;
        if (!ClearCommError(handler, &err, &cs)) { connected = false; return ""; }
        DWORD available = cs.cbInQue;
        if (available == 0) return "";

        char c;
        std::string result;
        DWORD bytesRead;
        while (available-- > 0 &&
               ReadFile(handler, &c, 1, &bytesRead, NULL) && bytesRead > 0) {
            if (c == '\n') break;
            if (c == '\r') continue;
            result += c;
        }
        return result;
    }

    // Escritura con timeout — nunca congela la UI
    bool write(const std::string& data) {
        if (!connected || handler == INVALID_HANDLE_VALUE) return false;
        DWORD bytesWritten = 0;
        BOOL  ok = WriteFile(handler, data.c_str(),
                             (DWORD)data.size(), &bytesWritten, NULL);
        if (!ok || bytesWritten == 0) {
            // No tratar como desconexion — puede ser timeout puntual
            return false;
        }
        return true;
    }

    ~Serial() {
        if (handler != INVALID_HANDLE_VALUE)
            CloseHandle(handler);
    }
};