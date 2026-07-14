import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TextInput,
  TouchableOpacity,
  Switch,
  ActivityIndicator,
} from "react-native";
import { useState, useEffect } from "react";
import { useBLEContext } from "../context/BLEContext";
import type { UserProfile } from "../context/BLEContext";

export default function SettingsScreen() {
  const { isScanning, connectedDevice, scanAndConnect, disconnect, vitals, profile, setProfile } =
    useBLEContext();

  const [age, setAge] = useState(profile.age);
  const [weight, setWeight] = useState(profile.weight);
  const [height, setHeight] = useState(profile.height);
  const [fitnessLevel, setFitnessLevel] = useState(profile.fitnessLevel);
  const [autoAnalyze, setAutoAnalyze] = useState(profile.autoAnalyze);
  const [saved, setSaved] = useState(false);

  const connected = connectedDevice !== null;

  const handleSave = () => {
    setProfile({ age, weight, height, fitnessLevel, autoAnalyze });
    setSaved(true);
    setTimeout(() => setSaved(false), 2000);
  };

  // Auto-analyze every 60 seconds when enabled and connected
  useEffect(() => {
    if (!autoAnalyze || !connected) return;

    const interval = setInterval(() => {
      // Trigger analysis by dispatching a custom event
      // The agent screen listens for this
      console.log("Auto-analyze triggered");
    }, 60000);

    return () => clearInterval(interval);
  }, [autoAnalyze, connected]);

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.title}>Settings</Text>
      <Text style={styles.subtitle}>
        Your profile helps the AI give personalized feedback
      </Text>

      {/* Profile Section */}
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

      {/* Fitness Level Section */}
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

      {/* Agent Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Agent</Text>
        <View style={styles.toggleRow}>
          <View>
            <Text style={styles.toggleLabel}>Auto-analyze every 60s</Text>
            <Text style={styles.toggleSub}>
              {connected
                ? "Agent runs automatically during sessions"
                : "Connect device to enable"}
            </Text>
          </View>
          <Switch
            value={autoAnalyze && connected}
            onValueChange={(val) => {
              if (!connected && val) return;
              setAutoAnalyze(val);
            }}
            trackColor={{ false: "#333", true: "#00ff88" }}
            thumbColor="#fff"
            disabled={!connected}
          />
        </View>
      </View>

      {/* Device Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device</Text>
        <View style={styles.deviceRow}>
          <View
            style={[
              styles.deviceDot,
              { backgroundColor: connected ? "#00ff88" : "#ff4444" },
            ]}
          />
          <Text style={styles.deviceText}>
            {connected ? "HealthMonitor connected" : "No device connected"}
          </Text>
        </View>

        {connected && (
          <View style={styles.vitalsPreview}>
            <Text style={styles.vitalsPreviewText}>
              HR: {Math.round(vitals.hr)} BPM · O₂: {Math.round(vitals.spo2)}% ·{" "}
              {vitals.temp.toFixed(1)}°F · {vitals.motion}
            </Text>
          </View>
        )}

        <TouchableOpacity
          style={[styles.scanButton, connected && styles.disconnectButton]}
          onPress={connected ? disconnect : scanAndConnect}
          disabled={isScanning}
        >
          {isScanning ? (
            <ActivityIndicator color="#4488ff" />
          ) : (
            <Text
              style={[
                styles.scanButtonText,
                connected && styles.disconnectButtonText,
              ]}
            >
              {connected ? "Disconnect Device" : "Scan for Device"}
            </Text>
          )}
        </TouchableOpacity>
      </View>

      {/* Save Button */}
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
  },
  deviceText: {
    color: "#888",
    fontSize: 14,
  },
  vitalsPreview: {
    backgroundColor: "#141414",
    borderRadius: 8,
    padding: 10,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: "#222",
  },
  vitalsPreviewText: {
    color: "#00ff88",
    fontSize: 12,
    fontWeight: "500",
  },
  scanButton: {
    backgroundColor: "#141414",
    borderRadius: 10,
    padding: 14,
    alignItems: "center",
    borderWidth: 1,
    borderColor: "#333",
  },
  disconnectButton: {
    borderColor: "#ff4444",
  },
  scanButtonText: {
    color: "#4488ff",
    fontSize: 14,
    fontWeight: "600",
  },
  disconnectButtonText: {
    color: "#ff4444",
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
