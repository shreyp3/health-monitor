import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  ActivityIndicator,
} from "react-native";
import { useBLE } from "../hooks/useBLE";

function VitalCard({
  label,
  value,
  unit,
  color,
}: {
  label: string;
  value: string | number;
  unit: string;
  color: string;
}) {
  return (
    <View style={styles.card}>
      <Text style={styles.cardLabel}>{label}</Text>
      <Text style={[styles.cardValue, { color }]}>{value}</Text>
      <Text style={[styles.cardUnit, { color }]}>{unit}</Text>
    </View>
  );
}

export default function DashboardScreen() {
  const {
    isScanning,
    connectedDevice,
    vitals,
    error,
    scanAndConnect,
    disconnect,
  } = useBLE();

  const connected = connectedDevice !== null;

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.title}>Health Monitor</Text>

      {/* Connection status badge */}
      <View
        style={[
          styles.connectionBadge,
          { backgroundColor: connected ? "#1a3a1a" : "#3a1a1a" },
        ]}
      >
        <View
          style={[
            styles.connectionDot,
            { backgroundColor: connected ? "#00ff88" : "#ff4444" },
          ]}
        />
        <Text
          style={[
            styles.connectionText,
            { color: connected ? "#00ff88" : "#ff4444" },
          ]}
        >
          {connected
            ? "Device Connected"
            : isScanning
              ? "Scanning..."
              : "Device Disconnected"}
        </Text>
      </View>

      {/* Error message */}
      {error && (
        <View style={styles.errorBadge}>
          <Text style={styles.errorText}>{error}</Text>
        </View>
      )}

      {/* Connect / Disconnect button */}
      <TouchableOpacity
        style={[styles.connectButton, connected && styles.disconnectButton]}
        onPress={connected ? disconnect : scanAndConnect}
        disabled={isScanning}
      >
        {isScanning ? (
          <ActivityIndicator color="#000" />
        ) : (
          <Text style={styles.connectButtonText}>
            {connected ? "Disconnect" : "Connect to Device"}
          </Text>
        )}
      </TouchableOpacity>

      {/* Vitals grid */}
      <View style={styles.grid}>
        <VitalCard
          label="Heart Rate"
          value={vitals.hr > 0 ? Math.round(vitals.hr) : "--"}
          unit="BPM"
          color="#ff4444"
        />
        <VitalCard
          label="SpO2"
          value={vitals.spo2 > 0 ? Math.round(vitals.spo2) : "--"}
          unit="%"
          color="#4488ff"
        />
        <VitalCard
          label="Temperature"
          value={vitals.temp > 0 ? vitals.temp.toFixed(1) : "--"}
          unit="°F"
          color="#ffaa00"
        />
        <VitalCard
          label="Motion"
          value={vitals.motion || "--"}
          unit=""
          color="#00ff88"
        />
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: "#0a0a0a",
  },
  content: {
    padding: 24,
    paddingTop: 60,
  },
  title: {
    color: "#ffffff",
    fontSize: 28,
    fontWeight: "700",
    marginBottom: 20,
  },
  connectionBadge: {
    flexDirection: "row",
    alignItems: "center",
    padding: 12,
    borderRadius: 12,
    marginBottom: 12,
    gap: 8,
  },
  connectionDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
  },
  connectionText: {
    fontSize: 14,
    fontWeight: "600",
  },
  errorBadge: {
    backgroundColor: "#3a1a1a",
    borderRadius: 8,
    padding: 10,
    marginBottom: 12,
  },
  errorText: {
    color: "#ff4444",
    fontSize: 13,
  },
  connectButton: {
    backgroundColor: "#ffffff",
    borderRadius: 12,
    padding: 14,
    alignItems: "center",
    marginBottom: 24,
  },
  disconnectButton: {
    backgroundColor: "#1a1a1a",
    borderWidth: 1,
    borderColor: "#333",
  },
  connectButtonText: {
    color: "#000",
    fontSize: 15,
    fontWeight: "700",
  },
  grid: {
    flexDirection: "row",
    flexWrap: "wrap",
    gap: 16,
  },
  card: {
    backgroundColor: "#141414",
    borderRadius: 16,
    padding: 20,
    width: "47%",
    borderWidth: 1,
    borderColor: "#222",
  },
  cardLabel: {
    color: "#888",
    fontSize: 13,
    fontWeight: "500",
    marginBottom: 8,
  },
  cardValue: {
    fontSize: 36,
    fontWeight: "700",
    marginBottom: 4,
  },
  cardUnit: {
    fontSize: 14,
    fontWeight: "500",
    opacity: 0.8,
  },
});
