import { Stack } from "expo-router";
import { BLEProvider } from "./context/BLEContext";

export default function RootLayout() {
  return (
    <BLEProvider>
      <Stack screenOptions={{ headerShown: false }}>
        <Stack.Screen name="(tabs)" />
      </Stack>
    </BLEProvider>
  );
}
