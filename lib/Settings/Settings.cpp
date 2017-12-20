/**
  MQTT433gateway - MQTT 433.92 MHz radio gateway utilizing ESPiLight
  Project home: https://github.com/puuu/MQTT433gateway/

  The MIT License (MIT)

  Copyright (c) 2017 Jan Losinski

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#include <algorithm>

#include "Settings.h"

#include <ArduinoJson.h>
#include <FS.h>

#include <ArduinoSimpleLogging.h>

static inline bool any(std::initializer_list<bool> items) {
  return std::any_of(items.begin(), items.end(),
                     [](bool item) { return item; });
}

std::function<bool(const String &)> notEmpty() {
  return [](const String &str) { return str.length() > 0; };
}

template <typename T>
std::function<bool(const T &)> notZero() {
  return [](const T &val) { return val != 0; };
}

template <typename TVal, typename TKey>
bool setIfPresent(JsonObject &obj, TKey key, TVal &var,
                  const std::function<bool(const TVal &)> &validator =
                      std::function<bool(const TVal &)>()) {
  if (obj.containsKey(key)) {
    TVal tmp = obj.get<TVal>(key);
    if (tmp != var && (!validator || validator(tmp))) {
      var = tmp;
      return true;
    }
  }
  return false;
}

void Settings::registerChangeHandler(SettingType setting,
                                     const SettingCallbackFn &callback) {
  listeners.emplace_front(setting, callback);
}

void Settings::onConfigChange(SettingTypeSet typeSet) const {
  for (const auto &listener : listeners) {
    if (typeSet[listener.type]) {
      listener.callback(*this);
    }
  }
}

void Settings::load() {
  if (SPIFFS.exists(SETTINGS_FILE)) {
    File file = SPIFFS.open(SETTINGS_FILE, "r");
    String settingsContents = file.readStringUntil(SETTINGS_TERMINATOR);
    file.close();
    Logger.debug.println("FILE CONTENTS:");
    Logger.debug.println(settingsContents);

    deserialize(settingsContents, false);
  }

  // Fire for all
  onConfigChange(SettingTypeSet().set());
}

void Settings::save() {
  File file = SPIFFS.open(SETTINGS_FILE, "w");

  if (!file) {
    Logger.error.println(F("Opening settings file failed"));
  } else {
    serialize(file, false, true);
    file.close();
  }
}

Settings::~Settings() = default;

void Settings::updateOtaUrl(const String &otaUrl) { this->otaUrl = otaUrl; }

void Settings::serialize(Print &stream, bool pretty, bool sensible) const {
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();

  root[F("deviceName")] = this->deviceName;
  root[F("mdnsName")] = this->mdnsName;
  root[F("mqttReceiveTopic")] = this->mqttReceiveTopic;
  root[F("mqttSendTopic")] = this->mqttSendTopic;
  root[F("mqttOtaTopic")] = this->mqttOtaTopic;
  root[F("mqttBroker")] = this->mqttBroker;
  root[F("mqttBrokerPort")] = this->mqttBrokerPort;
  root[F("mqttUser")] = this->mqttUser;
  root[F("mqttRetain")] = this->mqttRetain;
  root[F("rfReceiverPin")] = this->rfReceiverPin;
  root[F("rfTransmitterPin")] = this->rfTransmitterPin;
  root[F("rfEchoMessages")] = this->rfEchoMessages;

  {
    DynamicJsonBuffer protoBuffer;
    JsonArray &parsedProtocols = protoBuffer.parseArray(this->rfProtocols);
    JsonArray &protos = root.createNestedArray(F("rfProtocols"));
    for (const auto proto : parsedProtocols) {
      protos.add(proto.as<String>());
    }
  }

  root[F("otaUrl")] = this->otaUrl;
  root[F("serialLogLevel")] = this->serialLogLevel;
  root[F("webLogLevel")] = this->webLogLevel;
  root[F("syslogLevel")] = this->syslogLevel;
  root[F("syslogHost")] = this->syslogHost;
  root[F("syslogPort")] = this->syslogPort;

  if (sensible) {
    root[F("mqttPassword")] = this->mqttPassword;
    root[F("configPassword")] = this->configPassword;
  }

  if (pretty) {
    root.prettyPrintTo(stream);
  } else {
    root.printTo(stream);
  }
}

void Settings::deserialize(const String &json, const bool fireCallbacks) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject &parsedSettings = jsonBuffer.parseObject(json);

  if (!parsedSettings.success()) {
    Logger.warning.println(F("Config parse failed!"));
    return;
  }

  SettingTypeSet changed;

  changed.set(
      BASE,
      any({setIfPresent(parsedSettings, F("deviceName"), deviceName,
                        notEmpty()),
           setIfPresent(parsedSettings, F("mdnsName"), mdnsName, notEmpty())}));

  changed.set(
      MQTT,
      any({setIfPresent(parsedSettings, F("mqttReceiveTopic"),
                        mqttReceiveTopic),
           setIfPresent(parsedSettings, F("mqttSendTopic"), mqttSendTopic),
           setIfPresent(parsedSettings, F("mqttOtaTopic"), mqttOtaTopic),
           setIfPresent(parsedSettings, F("mqttBroker"), mqttBroker,
                        notEmpty()),
           setIfPresent(parsedSettings, F("mqttBrokerPort"), mqttBrokerPort,
                        notZero<uint16_t>()),
           setIfPresent(parsedSettings, F("mqttUser"), mqttUser),
           setIfPresent(parsedSettings, F("mqttPassword"), mqttPassword,
                        notEmpty()),
           setIfPresent(parsedSettings, F("mqttRetain"), mqttRetain)}));

  changed.set(
      RF_CONFIG,
      any({setIfPresent(parsedSettings, F("rfReceiverPin"), rfReceiverPin),
           setIfPresent(parsedSettings, F("rfTransmitterPin"),
                        rfTransmitterPin)}));

  changed.set(RF_ECHO, (setIfPresent(parsedSettings, F("rfEchoMessages"),
                                     rfEchoMessages)));

  if (parsedSettings.containsKey(F("rfProtocols"))) {
    String buff;
    parsedSettings[F("rfProtocols")].printTo(buff);
    if (buff != rfProtocols) {
      rfProtocols = buff;
      changed.set(RF_PROTOCOL, true);
    }
  }

  changed.set(OTA, setIfPresent(parsedSettings, F("otaUrl"), otaUrl));

  changed.set(
      LOGGING,
      any({setIfPresent(parsedSettings, F("serialLogLevel"), serialLogLevel),
           setIfPresent(parsedSettings, F("webLogLevel"), webLogLevel)}));

  changed.set(WEB_CONFIG, setIfPresent(parsedSettings, F("configPassword"),
                                       configPassword, notEmpty()));

  changed.set(SYSLOG,
              any({setIfPresent(parsedSettings, F("syslogLevel"), syslogLevel),
                   setIfPresent(parsedSettings, F("syslogHost"), syslogHost),
                   setIfPresent(parsedSettings, F("syslogPort"), syslogPort)}));

  if (fireCallbacks) {
    onConfigChange(changed);
  }
}

void Settings::reset() {
  if (SPIFFS.exists(SETTINGS_FILE)) {
    SPIFFS.remove(SETTINGS_FILE);
  }
}
