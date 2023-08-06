#pragma once

#include "wled.h"


class TwangUsermod : public Usermod {

  private:

    bool firstLoop = true;
    bool initDone = false;
    bool enabled = true;

    // strings to reduce flash memory usage (used more than twice)
    static const char _name[];
    static const char _twangEnabled[];

    void enable(bool enable) {
      enabled = enable;
    }

  public:

    // gets called once at boot. Do all initialization that doesn't depend on
    // network here
    void setup() {
      
      initDone = true;
    
    }

    // gets called every time WiFi is (re-)connected. Initialize own network
    // interfaces here
    void connected() {}

    /*
     * Da loop.
     */
    void loop() {
      if (!enabled || strip.isUpdating() || currentPreset>0) return;

      unsigned long now = millis();
      uint8_t currentMode = strip.getMainSegment().mode;
      uint8_t currentPalette = strip.getMainSegment().palette;

      
    }

    /*
     * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
     * Creating an "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
     * Below it is shown how this could be used for e.g. a light sensor
     */
    void addToJsonInfo(JsonObject& root) {
      JsonObject user = root["u"];
      if (user.isNull()) {
        user = root.createNestedObject("u");
      }

      JsonArray infoArr = user.createNestedArray(FPSTR(_name));  // name

      String uiDomString = F("<button class=\"btn btn-xs\" onclick=\"requestJson({");
      uiDomString += FPSTR(_name);
      uiDomString += F(":{");
      uiDomString += FPSTR(_twangEnabled);
      uiDomString += enabled ? F(":false}});\">") : F(":true}});\">");
      uiDomString += F("<i class=\"icons ");
      uiDomString += enabled ? "on" : "off";
      uiDomString += F("\">&#xe08f;</i></button>");
      infoArr.add(uiDomString);
    }

    /*
     * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    //void addToJsonState(JsonObject& root) {
    //}

    /*
     * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void readFromJsonState(JsonObject& root) {
      if (!initDone) return;  // prevent crash on boot applyPreset()
      bool en = enabled;
      JsonObject um = root[FPSTR(_name)];
      if (!um.isNull()) {
        if (um[FPSTR(_twangEnabled)].is<bool>()) {
          en = um[FPSTR(_twangEnabled)].as<bool>();
        } else {
          String str = um[FPSTR(_twangEnabled)]; // checkbox -> off or on
          en = (bool)(str!="off"); // off is guaranteed to be present
        }
        if (en != enabled) enable(en);
      }
    }

    /*
     * addToConfig() can be used to add custom persistent settings to the cfg.json file in the "um" (usermod) object.
     * It will be called by WLED when settings are actually saved (for example, LED settings are saved)
     * If you want to force saving the current state, use serializeConfig() in your loop().
     * 
     * CAUTION: serializeConfig() will initiate a filesystem write operation.
     * It might cause the LEDs to stutter and will cause flash wear if called too often.
     * Use it sparingly and always in the loop, never in network callbacks!
     * 
     * addToConfig() will also not yet add your setting to one of the settings pages automatically.
     * To make that work you still have to add the setting to the HTML, xml.cpp and set.cpp manually.
     * 
     * I highly recommend checking out the basics of ArduinoJson serialization and deserialization in order to use custom settings!
     */
    void addToConfig(JsonObject& root) {
      // we add JSON object: {"Autosave": {"autoSaveAfterSec": 10, "autoSavePreset": 99}}
      JsonObject top = root.createNestedObject(FPSTR(_name)); // usermodname
      top[FPSTR(_twangEnabled)]     = enabled;
      //top[FPSTR(_autoSaveAfterSec)]    = autoSaveAfterSec;  // usermodparam
      //top[FPSTR(_autoSavePreset)]      = autoSavePreset;    // usermodparam
      //top[FPSTR(_autoSaveApplyOnBoot)] = applyAutoSaveOnBoot;
      DEBUG_PRINTLN(F("Autosave config saved."));
    }

    /*
     * readFromConfig() can be used to read back the custom settings you added with addToConfig().
     * This is called by WLED when settings are loaded (currently this only happens once immediately after boot)
     * 
     * readFromConfig() is called BEFORE setup(). This means you can use your persistent values in setup() (e.g. pin assignments, buffer sizes),
     * but also that if you want to write persistent values to a dynamic buffer, you'd need to allocate it here instead of in setup.
     * If you don't know what that is, don't fret. It most likely doesn't affect your use case :)
     * 
     * The function should return true if configuration was successfully loaded or false if there was no configuration.
     */
    bool readFromConfig(JsonObject& root) {
      // we look for JSON object: {"Autosave": {"enabled": true, "autoSaveAfterSec": 10, "autoSavePreset": 250, ...}}
      JsonObject top = root[FPSTR(_name)];
      if (top.isNull()) {
        DEBUG_PRINT(FPSTR(_name));
        DEBUG_PRINTLN(F(": No config found. (Using defaults.)"));
        return false;
      }

      enabled             = top[FPSTR(_twangEnabled)] | enabled;
      //autoSaveAfterSec    = top[FPSTR(_autoSaveAfterSec)] | autoSaveAfterSec;
      //autoSaveAfterSec    = (uint16_t) min(3600,max(10,(int)autoSaveAfterSec)); // bounds checking
      //autoSavePreset      = top[FPSTR(_autoSavePreset)] | autoSavePreset;
      //autoSavePreset      = (uint8_t) min(250,max(100,(int)autoSavePreset)); // bounds checking
      //applyAutoSaveOnBoot = top[FPSTR(_autoSaveApplyOnBoot)] | applyAutoSaveOnBoot;
      DEBUG_PRINT(FPSTR(_name));
      DEBUG_PRINTLN(F(" config (re)loaded."));

      // use "return !top["newestParameter"].isNull();" when updating Usermod with new features
      return true;
  }

    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId() {
      return USERMOD_ID_AUTO_SAVE;
    }
};

// strings to reduce flash memory usage (used more than twice)
const char AutoSaveUsermod::_name[]                PROGMEM = "Twang";
const char AutoSaveUsermod::_twangEnabled[]     PROGMEM = "enabled";
//const char AutoSaveUsermod::_autoSaveAfterSec[]    PROGMEM = "autoSaveAfterSec";
//const char AutoSaveUsermod::_autoSavePreset[]      PROGMEM = "autoSavePreset";
//const char AutoSaveUsermod::_autoSaveApplyOnBoot[] PROGMEM = "autoSaveApplyOnBoot";
