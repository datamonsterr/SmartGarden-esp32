import os

from SCons.Script import Import


Import("env")


def _parse_env_file(path):
    data = {}
    with open(path, "r", encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            if "=" not in line:
                continue
            key, value = line.split("=", 1)
            key = key.strip()
            value = value.strip()
            if not key:
                continue

            # Support quoted values: WIFI_SSID="My Wifi"
            if len(value) >= 2 and ((value[0] == '"' and value[-1] == '"') or (value[0] == "'" and value[-1] == "'")):
                value = value[1:-1]

            data[key] = value
    return data


def _c_escape_string(value):
    # Escape for inclusion as a C++ string literal.
    return (
        value.replace("\\", "\\\\")
        .replace('"', "\\\"")
        .replace("\r", "")
        .replace("\n", "\\n")
    )


project_dir = env.subst("$PROJECT_DIR")
env_path = os.path.join(project_dir, ".env")

if not os.path.isfile(env_path):
    # No .env present: do nothing (developers can still use include/Secrets.h manually).
    print("[secrets] No .env found; skipping Secrets.h generation")
else:
    values = _parse_env_file(env_path)

    # WIFI_PASSWORD may legitimately be empty (e.g. Wokwi-GUEST).
    required_keys = ["WIFI_SSID", "TB_HOST", "TB_PORT", "TB_ACCESS_TOKEN"]
    missing = [key for key in required_keys if key not in values or values[key] == ""]

    if missing:
        print("[secrets] .env is missing keys: " + ", ".join(missing))
        print("[secrets] Skipping Secrets.h generation (fix .env or use include/Secrets.h)")
    else:
        try:
            tb_port = int(values["TB_PORT"])
        except ValueError:
            print("[secrets] TB_PORT must be a number. Skipping Secrets.h generation")
            tb_port = None

        if tb_port is not None:
            include_dir = os.path.join(project_dir, "include")
            os.makedirs(include_dir, exist_ok=True)
            secrets_h_path = os.path.join(include_dir, "Secrets.h")

            # IMPORTANT: When using .format(), literal '{' and '}' must be escaped as '{{' and '}}'.
            wifi_password = values.get("WIFI_PASSWORD", "")

            content = """#pragma once

// AUTO-GENERATED from `.env` by `scripts/pio_env_to_secrets.py`.
// Do not commit this file. It is gitignored.

namespace secrets {{

constexpr const char* kWifiSsid = \"{wifi_ssid}\";
constexpr const char* kWifiPassword = \"{wifi_password}\";

constexpr const char* kThingsBoardHost = \"{tb_host}\";
constexpr unsigned short kThingsBoardPort = {tb_port};

// Use the DEVICE Access Token as MQTT username.
constexpr const char* kThingsBoardAccessToken = \"{tb_token}\";

}}  // namespace secrets
""".format(
                wifi_ssid=_c_escape_string(values["WIFI_SSID"]),
                wifi_password=_c_escape_string(wifi_password),
                tb_host=_c_escape_string(values["TB_HOST"]),
                tb_port=tb_port,
                tb_token=_c_escape_string(values["TB_ACCESS_TOKEN"]),
            )

            with open(secrets_h_path, "w", encoding="utf-8", newline="\n") as handle:
                handle.write(content)

            print("[secrets] Generated include/Secrets.h from .env")
