import SwiftUI

@main
struct ProbeApp: App {
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}

struct ContentView: View {
    @State private var report = "Not checked yet."

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(alignment: .leading, spacing: 16) {
                    Text("""
                    1. Enable the driver: Settings > General > Drivers (or Settings > Usbmux Probe > Drivers).
                    2. Plug an iPhone into the iPad's USB-C port.
                    3. Come back here and tap Check driver.
                    """)
                    .font(.callout)

                    Button("Check driver") {
                        report = ProbeBridge.check()
                    }
                    .buttonStyle(.borderedProminent)

                    Button("Open Settings") {
                        if let url = URL(string: UIApplication.openSettingsURLString) {
                            UIApplication.shared.open(url)
                        }
                    }
                    .buttonStyle(.bordered)

                    Text(report)
                        .font(.system(.footnote, design: .monospaced))
                        .textSelection(.enabled)
                        .frame(maxWidth: .infinity, alignment: .leading)
                }
                .padding()
            }
            .navigationTitle("Usbmux Probe")
        }
    }
}
