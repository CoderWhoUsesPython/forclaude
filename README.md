# Usbmux Probe

An iPadOS app with an embedded DriverKit USB extension. It answers one question: **does a
DriverKit extension load on an M-series iPad when the IPA is re-signed with a free Apple ID?**

The dext matches any Apple (vendor 0x05AC) USB device, opens it, and exposes a small user client.
The app reads back what the driver saw. It does not speak usbmux yet. That comes after the driver loads.

## Build (no Mac needed)

1. Push this folder to a public GitHub repo (public repos get free macOS runner minutes).
2. Actions > "Build unsigned IPA" > Run workflow. Leave `archs` as `arm64 arm64e` for a fat
   binary, or set `arm64` if arm64e fails to build.
3. Download the `UsbmuxProbe-ipa` artifact. If the build fails, download `build-log` instead.

The project is untested. The extension sources were written without a compiler, so expect to
iterate on the first CI run.

## Run

1. Sideload `UsbmuxProbe.ipa` with SideStore.
2. Settings > General > Drivers (or Settings > Usbmux Probe > Drivers): enable the driver.
   If no driver is listed, iPadOS did not register the dext. That is the result you are testing for.
3. Plug an iPhone into the iPad's USB-C port, open the app, tap **Check driver**.

## Reading the result

| Report says | Meaning |
| --- | --- |
| No driver listed in Settings | Signing dropped the DriverKit entitlements, or the dext was not embedded |
| "No running UsbmuxProbe driver found" | Driver not enabled, or no Apple USB device attached |
| `IOServiceOpen failed` | Driver runs but the app lacks `communicates-with-drivers` |
| `device-open-result=0x0` | USB access works from a free-account install |
