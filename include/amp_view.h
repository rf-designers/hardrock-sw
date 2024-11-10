#pragma once

#include "amp_state.h"
#include "hr500_displays.h"

enum class view {
    left_panel,
    right_panel
};

enum class refresh_item : uint32_t {
    view_item_fwd_alert = 0x00010000,
    view_item_rfl_alert = 0x00020000,
    view_item_drive_alert = 0x00040000,
    view_item_voltage_alert = 0x00080000,
    view_item_current_alert = 0x00100000,
    view_item_temperature = 0x00200000,
};

struct amp_view {
    amplifier* amp = nullptr;

    void refresh() {
        if (has_changes(view::left_panel)) {
            //            Serial.println("left panel has changes");
            left_panel_refresh();
        }
        if (has_changes(view::right_panel)) {
            //            Serial.println("right panel has changes");
            right_panel_refresh();
        }
    }

    volatile uint32_t changes = 0xFFFFFFFF;

    bool has_changes(const view type) const {
        return type == view::left_panel ? (changes >> 16) > 0 : (changes & 0xFFFF) > 0;
    }

    inline bool has_change(refresh_item item) {
        return (changes & static_cast<uint32_t>(item)) == static_cast<uint32_t>(item);
    }

    void left_panel_refresh() {
        if (changes >> 16 == 0xFFFF) {
            //            Serial.println("redrawing all left panel");
            draw_meter();
        } else {
            // piece by piece
            if (has_change(refresh_item::view_item_fwd_alert)) {
                amp->lcd[0].fill_rect(20, 34, 25, 10, amp->state.colors.named.alarm[amp->state.alerts[alert_fwd_pwr]]);
            }
            if (has_change(refresh_item::view_item_rfl_alert)) {
                amp->lcd[0].fill_rect(84, 34, 25, 10, amp->state.colors.named.alarm[amp->state.alerts[alert_rfl_pwr]]);
            }
            if (has_change(refresh_item::view_item_drive_alert)) {
                amp->lcd[0].fill_rect(148, 34, 25, 10,
                                      amp->state.colors.named.alarm[amp->state.alerts[alert_drive_pwr]]);
            }
            if (has_change(refresh_item::view_item_voltage_alert)) {
                amp->lcd[0].fill_rect(212, 34, 25, 10, amp->state.colors.named.alarm[amp->state.alerts[alert_vdd]]);
            }
            if (has_change(refresh_item::view_item_current_alert)) {
                amp->lcd[0].fill_rect(276, 34, 25, 10, amp->state.colors.named.alarm[amp->state.alerts[alert_idd]]);
            }
            if (has_change(refresh_item::view_item_temperature)) {
                // erase the old one
                amp->lcd[0].draw_string(amp->state.TEMPbuff, 237, 203, 2, DGRAY);

                if (amp->state.temp_in_celsius) {
                    sprintf(amp->state.TEMPbuff, "%d&C", amp->state.temp_read);
                } else {
                    sprintf(amp->state.TEMPbuff, "%d&F", amp->state.temp_read);
                }

                const auto temp = amp->state.temperature.get();
                uint8_t temp_level = 1;
                if (temp > 650)
                    temp_level = 3;
                else if (temp > 500)
                    temp_level = 2;

                amp->lcd[0].draw_string(amp->state.TEMPbuff, 237, 203, 2, amp->state.colors.named.alarm[temp_level]);
            }
        }
        changes &= 0x0000FFFF;
    }

    void right_panel_refresh() {
        if ((changes & 0xFFFF) == 0xFFFF) {
            if (amp->state.is_menu_active) {
                //                Serial.println("redrawing menu");
                draw_menu();
            } else {
                //                Serial.println("redrawing right panel");
                draw_home();
            }
        } else {
            // piece by piece
        }
        changes &= 0xFFFF0000;
    }

    void item_changed(refresh_item item) {
        //        Serial.println("item changed");
        changes |= static_cast<uint32_t>(item);
    }
};
