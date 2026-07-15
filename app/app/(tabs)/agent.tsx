import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  ActivityIndicator,
} from "react-native";
import { useState } from "react";
import { useBLEContext } from "../context/BLEContext";

type SessionReading = {
  vitals: {
    hr: number;
    spo2: number;
    temp: number;
    motion: string;
  };
  agentResponse: string;
  timestamp: string;
};

type Message = {
  id: string;
  role: "agent";
  content: string;
  timestamp: string;
  type: "normal" | "warning" | "info";
};

function MessageCard({ message }: { message: Message }) {
  const icons = { normal: "✓", warning: "⚠", info: "ℹ" };
  const colors = { normal: "#00ff88", warning: "#ffaa00", info: "#4488ff" };

  return (
    <View
      style={[styles.messageCard, { borderLeftColor: colors[message.type] }]}
    >
      <View style={styles.messageHeader}>
        <Text style={[styles.messageIcon, { color: colors[message.type] }]}>
          {icons[message.type]}
        </Text>
        <Text style={styles.messageTime}>{message.timestamp}</Text>
      </View>
      <Text style={styles.messageContent}>{message.content}</Text>
    </View>
  );
}

export default function AgentScreen() {
  const { vitals, connectedDevice, profile } = useBLEContext();
  const [messages, setMessages] = useState<Message[]>([]);
  const [loading, setLoading] = useState(false);
  const [sessionHistory, setSessionHistory] = useState<SessionReading[]>([]);

  const connected = connectedDevice !== null;

  const getTimestamp = () => {
    const now = new Date();
    return now.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
  };

  const analyzeVitals = async () => {
    setLoading(true);

    try {
      const historyContext =
        sessionHistory.length > 0
          ? `Previous readings this session:\n${sessionHistory
              .map(
                (r) =>
                  `${r.timestamp} - HR: ${r.vitals.hr} BPM, SpO2: ${r.vitals.spo2}%, Temp: ${r.vitals.temp}°F, Motion: ${r.vitals.motion}\nYour response: ${r.agentResponse}`,
              )
              .join("\n\n")}\n\n`
          : "";

      const currentVitals = connected
        ? vitals
        : { hr: 72, spo2: 98, temp: 98.4, motion: "Resting" };

      const response = await fetch(
        "https://api.groq.com/openai/v1/chat/completions",
        {
          method: "POST",
          headers: {
            "Content-Type": "application/json",
            Authorization: `Bearer ${process.env.EXPO_PUBLIC_GROQ_API_KEY}`,
          },
          body: JSON.stringify({
            model: "llama-3.3-70b-versatile",
            max_tokens: 150,
            messages: [
              {
                role: "system",
                content: `You are a health monitoring AI assistant. You analyze vital signs and provide clear, 
                          concise health feedback. You are not a doctor and always remind users to consult healthcare 
                          professionals for medical advice. Keep responses under 3 sentences. Be direct and helpful.
                          You have memory of previous readings this session and should reference trends when relevant. 
                          ${profile.age ? `The user is ${profile.age} years old.` : ""}
                          ${profile.weight ? `They weigh ${profile.weight} lbs.` : ""}
                          ${profile.height ? `Their height is ${profile.height} ft.` : ""}
                          ${profile.fitnessLevel ? `Their fitness level is ${profile.fitnessLevel}.` : ""}
                          The device being used is a prototype wearable with consumer-grade sensors, not clinical equipment.
                          Readings will fluctuate, SpO2 may show occasional dips, heart rate may vary by 10-15 BPM between readings,
                          and temperature may read lower than core body temperature. This is expected and normal for this device.
                          Do NOT flag these normal fluctuations as warnings or concerning. Only flag something as [warning] if 
                          readings are consistently and severely abnormal (HR below 40 or above 140, SpO2 below 88, temp above 103F).
                          For everything else, default to [normal] or [info]. Never suggest the user seek medical attention 
                          based on minor fluctuations.
                          Use this context to personalize your health insights.
                          Classify your response as one of: normal (vitals look good), warning (something needs attention), 
                          or info (general health tip). Start your response with [normal], [warning], or [info].`,
              },
              {
                role: "user",
                content: `${historyContext}Current vital signs:
              Heart Rate: ${currentVitals.hr} BPM
              SpO2: ${currentVitals.spo2}%
              Temperature: ${currentVitals.temp}°F
              Motion Level: ${currentVitals.motion}
              
              Based on the current readings${sessionHistory.length > 0 ? " and the session history above" : ""}, provide a brief health insight.`,
              },
            ],
          }),
        },
      );

      if (!response.ok) {
        const errorText = await response.text();
        throw new Error(`API error: ${response.status} — ${errorText}`);
      }

      const data = await response.json();
      const content = data.choices[0].message.content || "";

      let type: "normal" | "warning" | "info" = "info";
      let cleanContent = content;

      if (content.startsWith("[normal]")) {
        type = "normal";
        cleanContent = content.replace("[normal]", "").trim();
      } else if (content.startsWith("[warning]")) {
        type = "warning";
        cleanContent = content.replace("[warning]", "").trim();
      } else if (content.startsWith("[info]")) {
        type = "info";
        cleanContent = content.replace("[info]", "").trim();
      }

      const timestamp = getTimestamp();

      setSessionHistory((prev) => [
        ...prev,
        {
          vitals: currentVitals,
          agentResponse: cleanContent,
          timestamp,
        },
      ]);

      const newMessage: Message = {
        id: Date.now().toString(),
        role: "agent",
        content: cleanContent,
        timestamp,
        type,
      };

      setMessages((prev) => [newMessage, ...prev]);
    } catch (error: any) {
      console.error("Groq API error:", error);
      const errorMessage: Message = {
        id: Date.now().toString(),
        role: "agent",
        content: `Error: ${error.message}`,
        timestamp: getTimestamp(),
        type: "warning",
      };
      setMessages((prev) => [errorMessage, ...prev]);
    } finally {
      setLoading(false);
    }
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>AI Agent</Text>
        <Text style={styles.subtitle}>Powered by Llama 3</Text>
      </View>

      <View style={styles.vitalsRow}>
        <Text style={styles.vitalsLabel}>
          {connected
            ? "Live readings:"
            : "Demo readings (connect device for live data):"}
        </Text>
        <Text style={styles.vitalsText}>
          ❤ {connected ? Math.round(vitals.hr) : 72} BPM · O₂{" "}
          {connected ? Math.round(vitals.spo2) : 98}% ·{" "}
          {connected ? vitals.temp.toFixed(1) : "98.4"}°F ·{" "}
          {connected ? vitals.motion : "Resting"} motion
        </Text>
      </View>

      <TouchableOpacity
        style={[styles.analyzeButton, loading && styles.analyzeButtonDisabled]}
        onPress={analyzeVitals}
        disabled={loading}
      >
        {loading ? (
          <ActivityIndicator color="#000" />
        ) : (
          <Text style={styles.analyzeButtonText}>Analyze My Vitals</Text>
        )}
      </TouchableOpacity>

      <ScrollView
        style={styles.messagesContainer}
        contentContainerStyle={styles.messagesContent}
      >
        {messages.length === 0 ? (
          <View style={styles.emptyState}>
            <Text style={styles.emptyStateText}>
              Tap "Analyze My Vitals" to get AI feedback on your current
              readings.
            </Text>
          </View>
        ) : (
          messages.map((message) => (
            <MessageCard key={message.id} message={message} />
          ))
        )}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: "#0a0a0a",
    padding: 24,
    paddingTop: 60,
  },
  header: {
    marginBottom: 20,
  },
  title: {
    color: "#ffffff",
    fontSize: 28,
    fontWeight: "700",
  },
  subtitle: {
    color: "#888",
    fontSize: 13,
    marginTop: 2,
  },
  vitalsRow: {
    backgroundColor: "#141414",
    borderRadius: 12,
    padding: 14,
    marginBottom: 16,
    borderWidth: 1,
    borderColor: "#222",
  },
  vitalsLabel: {
    color: "#888",
    fontSize: 12,
    marginBottom: 4,
  },
  vitalsText: {
    color: "#fff",
    fontSize: 13,
    fontWeight: "500",
  },
  analyzeButton: {
    backgroundColor: "#ffffff",
    borderRadius: 12,
    padding: 16,
    alignItems: "center",
    marginBottom: 24,
  },
  analyzeButtonDisabled: {
    opacity: 0.5,
  },
  analyzeButtonText: {
    color: "#000",
    fontSize: 16,
    fontWeight: "700",
  },
  messagesContainer: {
    flex: 1,
  },
  messagesContent: {
    gap: 12,
  },
  messageCard: {
    backgroundColor: "#141414",
    borderRadius: 12,
    padding: 16,
    borderWidth: 1,
    borderColor: "#222",
    borderLeftWidth: 3,
  },
  messageHeader: {
    flexDirection: "row",
    alignItems: "center",
    marginBottom: 8,
    gap: 8,
  },
  messageIcon: {
    fontSize: 14,
    fontWeight: "700",
  },
  messageTime: {
    color: "#888",
    fontSize: 12,
  },
  messageContent: {
    color: "#ddd",
    fontSize: 14,
    lineHeight: 20,
  },
  emptyState: {
    alignItems: "center",
    paddingTop: 40,
  },
  emptyStateText: {
    color: "#555",
    fontSize: 14,
    textAlign: "center",
    lineHeight: 22,
  },
});
