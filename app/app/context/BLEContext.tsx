import React, { createContext, useContext, useState, useEffect } from "react";
import { useBLE, VitalsData } from "../hooks/useBLE";
import { Device } from "react-native-ble-plx";

// History entry with timestamp
export type VitalsHistoryEntry = {
  vitals: VitalsData;
  timestamp: number;
};

type BLEContextType = {
  isScanning: boolean;
  connectedDevice: Device | null;
  vitals: VitalsData;
  error: string | null;
  scanAndConnect: () => Promise<void>;
  disconnect: () => Promise<void>;
  history: VitalsHistoryEntry[];
};

const BLEContext = createContext<BLEContextType | null>(null);

export function BLEProvider({ children }: { children: React.ReactNode }) {
  const ble = useBLE();
  const [history, setHistory] = useState<VitalsHistoryEntry[]>([]);

  // Every time vitals update, add to history
  // Keep last 20 entries
  useEffect(() => {
    if (ble.vitals.hr > 0 || ble.vitals.spo2 > 0) {
      setHistory((prev) => {
        const newEntry: VitalsHistoryEntry = {
          vitals: ble.vitals,
          timestamp: Date.now(),
        };
        const updated = [...prev, newEntry];
        return updated.slice(-20);
      });
    }
  }, [ble.vitals]);

  return (
    <BLEContext.Provider value={{ ...ble, history }}>
      {children}
    </BLEContext.Provider>
  );
}

export function useBLEContext() {
  const context = useContext(BLEContext);
  if (!context)
    throw new Error("useBLEContext must be used within BLEProvider");
  return context;
}
