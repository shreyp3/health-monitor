import { View, Text, StyleSheet, ScrollView, Dimensions } from "react-native";
import { LineChart } from "react-native-chart-kit";

const screenWidth = Dimensions.get("window").width;

const FAKE_HISTORY = {
  heartRate: [68, 72, 75, 71, 69, 74, 78, 76, 72, 70],
  spo2: [97, 98, 98, 99, 97, 98, 96, 98, 99, 98],
  temperature: [98.1, 98.3, 98.4, 98.6, 98.4, 98.2, 98.5, 98.3, 98.4, 98.2],
  labels: ["0m", "1m", "2m", "3m", "4m", "5m", "6m", "7m", "8m", "9m"],
};

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
  return (
    <View style={styles.chartCard}>
      <Text style={styles.chartTitle}>{title}</Text>
      <LineChart
        data={{
          labels,
          datasets: [{ data }],
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
  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.title}>Session History</Text>
      <Text style={styles.subtitle}>Last 10 minutes</Text>

      <ChartCard
        title="Heart Rate"
        data={FAKE_HISTORY.heartRate}
        labels={FAKE_HISTORY.labels}
        color="#ff4444"
        unit="BPM"
      />
      <ChartCard
        title="SpO2"
        data={FAKE_HISTORY.spo2}
        labels={FAKE_HISTORY.labels}
        color="#4488ff"
        unit="%"
      />
      <ChartCard
        title="Temperature"
        data={FAKE_HISTORY.temperature}
        labels={FAKE_HISTORY.labels}
        color="#ffaa00"
        unit="°F"
      />
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
