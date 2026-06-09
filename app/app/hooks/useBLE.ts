import { useState, useCallback } from "react";
import { BleManager, Device, Characteristic } from "react-native-ble-plx";
import { Platform, PermissionsAndroid } from "react-native";
import { Buffer } from "buffer";

// ============================================================
// BLE CONFIGURATION
// Must match the UUIDs defined in your ESP32 firmware
// ============================================================
const HEALTH_MONITOR_SERVICE_UUID = "000000ff-0000-1000-8000-00805f9b34fb";
const HEALTH_MONITOR_CHAR_UUID = "0000ff01-0000-1000-8000-00805f9b34fb";
const DEVICE_NAME = "HealthMonitor";

// ============================================================
// VITALS DATA TYPE
// Matches the JSON structure sent by the ESP32
// ============================================================
export type VitalsData = {
  hr: number;
  spo2: number;
  temp: number;
  motion: string;
};

const manager = new BleManager();

export function useBLE() {
  const [isScanning, setIsScanning] = useState(false);
  const [connectedDevice, setConnectedDevice] = useState<Device | null>(null);
  const [vitals, setVitals] = useState<VitalsData>({
    hr: 0,
    spo2: 0,
    temp: 0,
    motion: "Unknown",
  });
  const [error, setError] = useState<string | null>(null);

  // ============================================================
  // REQUEST PERMISSIONS — required on Android
  // iOS permissions are handled via app.json infoPlist
  // ============================================================
  const requestPermissions = useCallback(async () => {
    if (Platform.OS === "android") {
      const granted = await PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
      ]);
      return Object.values(granted).every(
        (v) => v === PermissionsAndroid.RESULTS.GRANTED,
      );
    }
    return true;
  }, []);

  // ============================================================
  // SCAN AND CONNECT
  // Scans for the HealthMonitor device and connects to it
  // ============================================================
  const scanAndConnect = useCallback(async () => {
    setError(null);
    const hasPermission = await requestPermissions();
    if (!hasPermission) {
      setError("Bluetooth permission denied");
      return;
    }

    setIsScanning(true);

    manager.startDeviceScan(null, null, async (err, device) => {
      if (err) {
        setError(err.message);
        setIsScanning(false);
        return;
      }

      // Look for our specific device by name
      if (device?.name === DEVICE_NAME) {
        manager.stopDeviceScan();
        setIsScanning(false);

        try {
          const connected = await device.connect();
          await connected.discoverAllServicesAndCharacteristics();
          setConnectedDevice(connected);

          // Subscribe to vitals notifications
          connected.monitorCharacteristicForService(
            HEALTH_MONITOR_SERVICE_UUID,
            HEALTH_MONITOR_CHAR_UUID,
            (charErr, characteristic) => {
              if (charErr) {
                setError(charErr.message);
                return;
              }
              if (characteristic?.value) {
                try {
                  // Decode base64 value from BLE
                  const decoded = Buffer.from(
                    characteristic.value,
                    "base64",
                  ).toString("utf-8");
                  const parsed: VitalsData = JSON.parse(decoded);
                  setVitals(parsed);
                } catch (parseErr) {
                  setError("Failed to parse vitals data");
                }
              }
            },
          );

          // Handle disconnection
          connected.onDisconnected(() => {
            setConnectedDevice(null);
            setVitals({ hr: 0, spo2: 0, temp: 0, motion: "Unknown" });
          });
        } catch (connectErr: any) {
          setError(connectErr.message);
          setIsScanning(false);
        }
      }
    });

    // Stop scanning after 10 seconds if device not found
    setTimeout(() => {
      manager.stopDeviceScan();
      setIsScanning(false);
    }, 10000);
  }, [requestPermissions]);

  // ============================================================
  // DISCONNECT
  // ============================================================
  const disconnect = useCallback(async () => {
    if (connectedDevice) {
      await connectedDevice.cancelConnection();
      setConnectedDevice(null);
      setVitals({ hr: 0, spo2: 0, temp: 0, motion: "Unknown" });
    }
  }, [connectedDevice]);

  return {
    isScanning,
    connectedDevice,
    vitals,
    error,
    scanAndConnect,
    disconnect,
  };
}
