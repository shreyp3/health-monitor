import { View, Text, StyleSheet, ScrollView } from "react-native";

const FAKE_DATA = {
  heartRate: 72,
  spo2: 98,
  temperature: 98.4,
  motion: "Low",
  connected: false,
};

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
  const data = FAKE_DATA;

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.title}>Health Monitor</Text>

      <View
        style={[
          styles.connectionBadge,
          { backgroundColor: data.connected ? "#1a3a1a" : "#3a1a1a" },
        ]}
      >
        <View
          style={[
            styles.connectionDot,
            { backgroundColor: data.connected ? "#00ff88" : "#ff4444" },
          ]}
        />
        <Text
          style={[
            styles.connectionText,
            { color: data.connected ? "#00ff88" : "#ff4444" },
          ]}
        >
          {data.connected ? "Device Connected" : "Device Disconnected"}
        </Text>
      </View>

      <View style={styles.grid}>
        <VitalCard
          label="Heart Rate"
          value={data.heartRate}
          unit="BPM"
          color="#ff4444"
        />
        <VitalCard label="SpO2" value={data.spo2} unit="%" color="#4488ff" />
        <VitalCard
          label="Temperature"
          value={data.temperature}
          unit="°F"
          color="#ffaa00"
        />
        <VitalCard label="Motion" value={data.motion} unit="" color="#00ff88" />
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
    marginBottom: 24,
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
