/*
 * ImGui plugin example
 * Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
 * Copyright (C) 2021-2022 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: ISC
 */

#include "DistrhoUI.hpp"
#include "ResizeHandle.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class ImGuiPluginUI : public UI
{
    float fParams[2];
    ResizeHandle fResizeHandle;

    // ----------------------------------------------------------------------------------------------------------------

public:
    /**
       UI class constructor.
       The UI should be initialized to a default state that matches the plugin side.
     */
    ImGuiPluginUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true),
          fResizeHandle(this)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        // hide handle if UI is resizable
        if (isResizable())
            fResizeHandle.hide();
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // DSP/Plugin Callbacks

    /**
       A parameter has changed on the plugin side.@n
       This is called by the host to inform the UI about parameter changes.
     */
    void parameterChanged(uint32_t index, float value) override
    {

        printf("UI Param %d changed to %f\n", index, value);
        fParams[index] = value;
        repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Widget Callbacks

    /**
       ImGui specific onDisplay function.
     */
    void onImGuiDisplay() override
    {
        const float width = getWidth();
        const float height = getHeight();
        const float margin = 20.0f * getScaleFactor();

        ImGui::SetNextWindowPos(ImVec2(margin, margin));
        ImGui::SetNextWindowSize(ImVec2(width -  2 * margin, height - 2 * margin));

        if (ImGui::Begin("Future Arp", nullptr, ImGuiWindowFlags_NoResize))
        {

            if (ImGui::SliderFloat("Rate (BPM)", &fParams[0], 0.0f, 16.0f))
            {
                if (ImGui::IsItemActivated())
                    editParameter(0, true);

                setParameterValue(0, fParams[0]);
            }

            if (ImGui::SliderFloat("Note Lenght", &fParams[1], 0.01f, 1.0f))
            {
                if (ImGui::IsItemActivated())
                    editParameter(1, true);

                setParameterValue(1, fParams[1]);
            }

            if (ImGui::IsItemDeactivated())
            {
                editParameter(0, false);
                editParameter(1, false);
            }
        }
        ImGui::End();
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImGuiPluginUI)
};

// --------------------------------------------------------------------------------------------------------------------

UI *createUI()
{
    return new ImGuiPluginUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
