import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TextInput,
  TouchableOpacity,
  Switch,
} from "react-native";
import { useState } from "react";

export default function SettingsScreen() {
  const [age, setAge] = useState("");
  const [weight, setWeight] = useState("");
  const [height, setHeight] = useState("");
  const [fitnessLevel, setFitnessLevel] = useState<"low" | "moderate" | "high">(
    "moderate",
  );
  const [autoAnalyze, setAutoAnalyze] = useState(false);
  const [saved, setSaved] = useState(false);

  const handleSave = () => {
    setSaved(true);
    setTimeout(() => setSaved(false), 2000);
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.title}>Settings</Text>
      <Text style={styles.subtitle}>
        Your profile helps the AI give personalized feedback
      </Text>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Profile</Text>

        <View style={styles.inputGroup}>
          <Text style={styles.inputLabel}>Age</Text>
          <TextInput
            style={styles.input}
            value={age}
            onChangeText={setAge}
            placeholder="e.g. 22"
            placeholderTextColor="#555"
            keyboardType="numeric"
            keyboardAppearance="dark"
          />
        </View>

        <View style={styles.inputGroup}>
          <Text style={styles.inputLabel}>Weight (lbs)</Text>
          <TextInput
            style={styles.input}
            value={weight}
            onChangeText={setWeight}
            placeholder="e.g. 160"
            placeholderTextColor="#555"
            keyboardType="numeric"
            keyboardAppearance="dark"
          />
        </View>

        <View style={styles.inputGroup}>
          <Text style={styles.inputLabel}>Height (ft)</Text>
          <TextInput
            style={styles.input}
            value={height}
            onChangeText={setHeight}
            placeholder="e.g. 5.11"
            placeholderTextColor="#555"
            keyboardType="numeric"
            keyboardAppearance="dark"
          />
        </View>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Fitness Level</Text>
        <View style={styles.fitnessRow}>
          {(["low", "moderate", "high"] as const).map((level) => (
            <TouchableOpacity
              key={level}
              style={[
                styles.fitnessButton,
                fitnessLevel === level && styles.fitnessButtonActive,
              ]}
              onPress={() => setFitnessLevel(level)}
            >
              <Text
                style={[
                  styles.fitnessButtonText,
                  fitnessLevel === level && styles.fitnessButtonTextActive,
                ]}
              >
                {level.charAt(0).toUpperCase() + level.slice(1)}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Agent</Text>
        <View style={styles.toggleRow}>
          <View>
            <Text style={styles.toggleLabel}>Auto-analyze every 60s</Text>
            <Text style={styles.toggleSub}>
              Agent runs automatically during sessions
            </Text>
          </View>
          <Switch
            value={autoAnalyze}
            onValueChange={setAutoAnalyze}
            trackColor={{ false: "#333", true: "#00ff88" }}
            thumbColor="#fff"
          />
        </View>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device</Text>
        <View style={styles.deviceRow}>
          <View style={styles.deviceDot} />
          <Text style={styles.deviceText}>No device connected</Text>
        </View>
        <TouchableOpacity style={styles.scanButton}>
          <Text style={styles.scanButtonText}>Scan for Device</Text>
        </TouchableOpacity>
      </View>

      <TouchableOpacity style={styles.saveButton} onPress={handleSave}>
        <Text style={styles.saveButtonText}>
          {saved ? "Saved!" : "Save Settings"}
        </Text>
      </TouchableOpacity>
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
    paddingBottom: 40,
  },
  title: {
    color: "#ffffff",
    fontSize: 28,
    fontWeight: "700",
    marginBottom: 4,
  },
  subtitle: {
    color: "#888",
    fontSize: 13,
    marginBottom: 32,
  },
  section: {
    marginBottom: 32,
  },
  sectionTitle: {
    color: "#888",
    fontSize: 12,
    fontWeight: "600",
    textTransform: "uppercase",
    letterSpacing: 1,
    marginBottom: 12,
  },
  inputGroup: {
    marginBottom: 12,
  },
  inputLabel: {
    color: "#aaa",
    fontSize: 13,
    marginBottom: 6,
  },
  input: {
    backgroundColor: "#141414",
    borderRadius: 10,
    padding: 14,
    color: "#fff",
    fontSize: 15,
    borderWidth: 1,
    borderColor: "#222",
  },
  fitnessRow: {
    flexDirection: "row",
    gap: 10,
  },
  fitnessButton: {
    flex: 1,
    backgroundColor: "#141414",
    borderRadius: 10,
    padding: 12,
    alignItems: "center",
    borderWidth: 1,
    borderColor: "#222",
  },
  fitnessButtonActive: {
    backgroundColor: "#1a2a1a",
    borderColor: "#00ff88",
  },
  fitnessButtonText: {
    color: "#888",
    fontSize: 14,
    fontWeight: "600",
  },
  fitnessButtonTextActive: {
    color: "#00ff88",
  },
  toggleRow: {
    flexDirection: "row",
    alignItems: "center",
    justifyContent: "space-between",
    backgroundColor: "#141414",
    borderRadius: 12,
    padding: 16,
    borderWidth: 1,
    borderColor: "#222",
  },
  toggleLabel: {
    color: "#fff",
    fontSize: 14,
    fontWeight: "500",
  },
  toggleSub: {
    color: "#888",
    fontSize: 12,
    marginTop: 2,
  },
  deviceRow: {
    flexDirection: "row",
    alignItems: "center",
    gap: 8,
    marginBottom: 12,
  },
  deviceDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    backgroundColor: "#ff4444",
  },
  deviceText: {
    color: "#888",
    fontSize: 14,
  },
  scanButton: {
    backgroundColor: "#141414",
    borderRadius: 10,
    padding: 14,
    alignItems: "center",
    borderWidth: 1,
    borderColor: "#333",
  },
  scanButtonText: {
    color: "#4488ff",
    fontSize: 14,
    fontWeight: "600",
  },
  saveButton: {
    backgroundColor: "#fff",
    borderRadius: 12,
    padding: 16,
    alignItems: "center",
  },
  saveButtonText: {
    color: "#000",
    fontSize: 16,
    fontWeight: "700",
  },
});
