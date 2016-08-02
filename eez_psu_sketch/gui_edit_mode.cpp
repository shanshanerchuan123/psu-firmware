/*
* EEZ PSU Firmware
* Copyright (C) 2015-present, Envox d.o.o.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "psu.h"
#include "gui_data_snapshot.h"
#include "gui_edit_mode.h"
#include "gui_edit_mode_slider.h"
#include "gui_edit_mode_step.h"
#include "gui_edit_mode_keypad.h"

namespace eez {
namespace psu {
namespace gui {
namespace edit_mode {

static int tabIndex = PAGE_ID_EDIT_MODE_SLIDER;
static data::Cursor dataCursor;
static int dataId = -1;
static data::Value editValue;
static data::Value undoValue;
static data::Value minValue;
static data::Value maxValue;
static bool is_interactive_mode = true;

////////////////////////////////////////////////////////////////////////////////

void Snapshot::takeSnapshot() {
    if (edit_mode::isActive()) {
        editValue = edit_mode::getEditValue();

        getInfoText(infoText);

        interactiveModeSelector = isInteractiveMode() ? 0 : 1;

        step_index = edit_mode_step::getStepIndex();

        switch (edit_mode_keypad::getEditUnit()) {
        case data::UNIT_VOLT: keypadUnit = data::Value::ProgmemStr(PSTR("mV")); break;
        case data::UNIT_MILLI_VOLT: keypadUnit = data::Value::ProgmemStr(PSTR("V")); break;
        case data::UNIT_AMPER: keypadUnit = data::Value::ProgmemStr(PSTR("mA")); break;
        default: keypadUnit = data::Value::ProgmemStr(PSTR("A"));
        }

        edit_mode_keypad::getText(keypadText, sizeof(keypadText));
    }
}

data::Value Snapshot::get(uint8_t id) {
    if (id == DATA_ID_EDIT_VALUE) {
       return editValue;
    } else if (id == DATA_ID_EDIT_INFO) {
        return infoText;
    } else if (id == DATA_ID_EDIT_UNIT) {
        return keypadUnit;
    } else if (id == DATA_ID_EDIT_MODE_INTERACTIVE_MODE_SELECTOR) {
        return interactiveModeSelector;
    } else if (id == DATA_ID_EDIT_STEPS) {
        return step_index;
    }

    return data::Value();
}

bool Snapshot::isBlinking(data::Snapshot& snapshot, uint8_t id, bool &result) {
    if (id == DATA_ID_EDIT_VALUE) {
        result = (interactiveModeSelector == 1) && (editValue != edit_mode::getCurrentValue(snapshot));
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

bool isActive() {
    return dataId != -1;
}

void enter(int tabIndex_) {
	if (tabIndex_ != -1) {
		tabIndex = tabIndex_;
	}

    if (getActivePage() != tabIndex) {
        if (getActivePage() == PAGE_ID_MAIN) {
            dataCursor = found_widget_at_down.cursor;
            DECL_WIDGET(widget, found_widget_at_down.widgetOffset);
            dataId = widget->data;
        }

        editValue = data::currentSnapshot.get(dataCursor, dataId);
        undoValue = editValue;
        minValue = data::getMin(dataCursor, dataId);
        maxValue = data::getMax(dataCursor, dataId);

        if (tabIndex == PAGE_ID_EDIT_MODE_KEYPAD) {
            edit_mode_keypad::reset();
        }

        psu::enterTimeCriticalMode();

        showPage(tabIndex);
    }
}

void exit() {
    if (getActivePage() != PAGE_ID_MAIN) {
        dataId = -1;

        psu::leaveTimeCriticalMode();

        showPage(PAGE_ID_MAIN);
    }
}

void nonInteractiveSet() {
    data::set(dataCursor, dataId, editValue);
}

void nonInteractiveDiscard() {
    editValue = undoValue;
    data::set(dataCursor, dataId, undoValue);
}

bool isInteractiveMode() {
    return is_interactive_mode;
}

void toggleInteractiveMode() {
    is_interactive_mode = !is_interactive_mode;
    editValue = data::currentSnapshot.get(dataCursor, dataId);
    undoValue = editValue;
}

const data::Value& getEditValue() {
    return editValue;
}

data::Value getCurrentValue(data::Snapshot snapshot) {
    return snapshot.get(dataCursor, dataId);
}

const data::Value& getMin() {
    return minValue;
}

const data::Value& getMax() {
    return maxValue;
}

data::Unit getUnit() {
    return editValue.getUnit();
}

void setValue(float value_) {
    data::Value value = data::Value(value_, getUnit());
    if (is_interactive_mode || tabIndex == PAGE_ID_EDIT_MODE_KEYPAD) {
        if (!data::set(dataCursor, dataId, value)) {
            return;
        }
    }
    editValue = value;
}

bool isEditWidget(const WidgetCursor &widgetCursor) {
    DECL_WIDGET(widget, widgetCursor.widgetOffset);
    return widgetCursor.cursor == dataCursor && widget->data == dataId;
}

void getInfoText(char *infoText) {
    Channel &channel = Channel::get(dataCursor.iChannel);
    if (dataId == DATA_ID_VOLT) {
        sprintf_P(infoText, PSTR("Set Ch%d voltage [%d-%d V]"), channel.index, (int)minValue.getFloat(), (int)maxValue.getFloat());
    } else {
        sprintf_P(infoText, PSTR("Set Ch%d current [%d-%d A]"), channel.index, (int)minValue.getFloat(), (int)maxValue.getFloat());
    }
}

}
}
}
} // namespace eez::psu::gui::edit_mode
