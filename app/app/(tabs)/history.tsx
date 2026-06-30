import { View, Text, StyleSheet, ScrollView, Dimensions } from "react-native";
import { LineChart } from "react-native-chart-kit";
import { useBLEContext } from "../context/BLEContext";

const screenWidth = Dimensions.get("window").width;

const FALLBACK_DATA = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
const FALLBACK_LABELS = ["", "", "", "", "", "", "", "", "", ""];

function ChartCard({
  title,
  data,
  labels,
  color,
  unit,
}: {
  title: string;
  data: number[];
  labels: string[];
  color: string;
  unit: string;
}) {
  const safeData = data.length > 0 ? data : FALLBACK_DATA;
  const safeLabels = labels.length > 0 ? labels : FALLBACK_LABELS;

  return (
    <View style={styles.chartCard}>
      <Text style={styles.chartTitle}>{title}</Text>
      <LineChart
        data={{
          labels: safeLabels,
          datasets: [{ data: safeData }],
        }}
        width={screenWidth - 48}
        height={160}
        chartConfig={{
          backgroundColor: "#141414",
          backgroundGradientFrom: "#141414",
          backgroundGradientTo: "#141414",
          decimalPlaces: 1,
          color: () => color,
          labelColor: () => "#888",
          propsForDots: { r: "3", strokeWidth: "1", stroke: color },
        }}
        bezier
        style={styles.chart}
        withInnerLines={false}
        withOuterLines={false}
      />
      <Text style={[styles.chartUnit, { color }]}>{unit}</Text>
    </View>
  );
}

export default function HistoryScreen() {
  const { history, connectedDevice } = useBLEContext();
  const connected = connectedDevice !== null;

  // Extract arrays from history entries
  const heartRateData = history.map((e) => e.vitals.hr);
  const spo2Data = history.map((e) => e.vitals.spo2);
  const tempData = history.map((e) => e.vitals.temp);

  // Generate time labels from timestamps
  const labels = history.map((e, i) => {
    if (i === 0 || i === history.length - 1) {
      const seconds = Math.round((Date.now() - e.timestamp) / 1000);
      return `${seconds}s`;
    }
    return "";
  });

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.title}>Session History</Text>
      <Text style={styles.subtitle}>
        {connected
          ? `${history.length} readings collected`
          : "Connect device to collect data"}
      </Text>

      {history.length === 0 && (
        <View style={styles.emptyState}>
          <Text style={styles.emptyText}>
            {connected
              ? "Waiting for readings..."
              : "No data yet — connect your device on the Dashboard tab"}
          </Text>
        </View>
      )}

      {history.length > 1 && (
        <>
          <ChartCard
            title="Heart Rate"
            data={heartRateData}
            labels={labels}
            color="#ff4444"
            unit="BPM"
          />
          <ChartCard
            title="SpO2"
            data={spo2Data}
            labels={labels}
            color="#4488ff"
            unit="%"
          />
          <ChartCard
            title="Temperature"
            data={tempData}
            labels={labels}
            color="#ffaa00"
            unit="°F"
          />
        </>
      )}
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
    marginBottom: 4,
  },
  subtitle: {
    color: "#888",
    fontSize: 14,
    marginBottom: 24,
  },
  emptyState: {
    backgroundColor: "#141414",
    borderRadius: 16,
    padding: 24,
    alignItems: "center",
    borderWidth: 1,
    borderColor: "#222",
  },
  emptyText: {
    color: "#555",
    fontSize: 14,
    textAlign: "center",
    lineHeight: 22,
  },
  chartCard: {
    backgroundColor: "#141414",
    borderRadius: 16,
    padding: 16,
    marginBottom: 16,
    borderWidth: 1,
    borderColor: "#222",
  },
  chartTitle: {
    color: "#fff",
    fontSize: 16,
    fontWeight: "600",
    marginBottom: 12,
  },
  chart: {
    borderRadius: 8,
    marginLeft: -16,
  },
  chartUnit: {
    fontSize: 12,
    marginTop: 8,
    fontWeight: "500",
  },
});
