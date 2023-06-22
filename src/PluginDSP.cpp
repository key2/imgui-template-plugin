/*
 * ImGui plugin example
 * Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
 * Copyright (C) 2021-2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: ISC
 */

#include "DistrhoPlugin.hpp"
#include "extra/ValueSmoother.hpp"

START_NAMESPACE_DISTRHO

class ImGuiPluginDSP : public Plugin
{
    enum Parameters
    {
        kParamRate = 0,
        KParamNoteLen,
        kParamCount
    };

    struct arpaggio_s
    {
        float steps;
        bool note_on;
        uint8_t midi_status;
        uint8_t midi_velocity;
    } arpaggio[128];

    float fParams[kParamCount];

public:
    /**
       Plugin class constructor.@n
       You must set all parameter values to their defaults, matching ParameterRanges::def.
     */
    ImGuiPluginDSP()
        : Plugin(kParamCount, 0, 0) // parameters, programs, states
    {
        std::memset(fParams, 0, sizeof(fParams));
        fParams[kParamRate] = 1.0;
        fParams[KParamNoteLen] = 1.0;

        std::memset(arpaggio, 0, sizeof(arpaggio));
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // Information

    /**
       Get the plugin label.@n
       This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
     */
    const char *getLabel() const noexcept override
    {
        return "futureArp";
    }

    /**
       Get an extensive comment/description about the plugin.@n
       Optional, returns nothing by default.
     */
    const char *getDescription() const override
    {
        return "A Midi Arpeggiator with audio effect";
    }

    /**
       Get the plugin author/maker.
     */
    const char *getMaker() const noexcept override
    {
        return "Key2";
    }

    /**
       Get the plugin license (a single line of text or a URL).@n
       For commercial plugins this should return some short copyright information.
     */
    const char *getLicense() const noexcept override
    {
        return "ISC";
    }

    /**
       Get the plugin version, in hexadecimal.
       @see d_version()
     */
    uint32_t getVersion() const noexcept override
    {
        return d_version(1, 0, 0);
    }

    /**
       Get the plugin unique Id.@n
       This value is used by LADSPA, DSSI and VST plugin formats.
       @see d_cconst()
     */
    int64_t getUniqueId() const noexcept override
    {
        return d_cconst('m', 'M', 'A', 'r');
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Init

    /**
       Initialize the parameter @a index.@n
       This function will be called once, shortly after the plugin is created.
     */
    void initParameter(uint32_t index, Parameter &parameter) override
    {

        switch (index)
        {
        case kParamRate:
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 16.0f;
            parameter.ranges.def = 1.0f;
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Rate";
            parameter.shortName = "Rate";
            parameter.symbol = "Rate";
            parameter.unit = "bpm";
            break;

        case KParamNoteLen:
            parameter.ranges.min = 0.01f;
            parameter.ranges.max = 1.0f;
            parameter.ranges.def = 1.0f;
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Note Lenght";
            parameter.shortName = "NoteLen";
            parameter.symbol = "NoteLen";
            parameter.unit = "bpm";
            break;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Internal data

    /**
       Get the current value of a parameter.@n
       The host may call this function from any context, including realtime processing.
     */
    float getParameterValue(uint32_t index) const override
    {

        printf("requesting params %d returning %f\n", index, fParams[index]);
        return fParams[index];
    }

    /**
       Change a parameter value.@n
       The host may call this function from any context, including realtime processing.@n
       When a parameter is marked as automatable, you must ensure no non-realtime operations are performed.
       @note This function will only be called for parameter inputs.
     */
    void setParameterValue(uint32_t index, float value) override
    {
        // printf("set param %d valu %f\n", index, value);
        fParams[index] = value;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Audio/MIDI Processing

    /**
       Activate this plugin.
     */
    void activate() override
    {
        printf("actiavted\n");
    }

    /**
       Run/process function for plugins without MIDI input.
       @note Some parameters might be null if there are no audio inputs or outputs.
     */
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void run(const float **inputs, float **outputs, uint32_t frames,
             const MidiEvent *midiEvents, uint32_t midiEventCount) override
#else
    void run(const float **inputs, float **outputs, uint32_t frames) override
#endif
    {
        // get the left and right audio inputs
        const float *const inpL = inputs[0];
        const float *const inpR = inputs[1];

        // get the left and right audio outputs
        float *const outL = outputs[0];
        float *const outR = outputs[1];

        if (outputs[0] != inputs[0])
            std::memcpy(outputs[0], inputs[0], sizeof(float) * frames);

        if (outputs[1] != inputs[1])
            std::memcpy(outputs[1], inputs[1], sizeof(float) * frames);

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        for (uint32_t i = 0; i < midiEventCount; ++i)
        {
            /*
                Midi event: Status | Note | Velocity | xxx
            */

            uint8_t note = midiEvents[i].data[1];
            arpaggio[note].midi_status = midiEvents[i].data[0];
            arpaggio[note].midi_velocity = midiEvents[i].data[2];

            printf("node %d status %02x\n", note, arpaggio[note].midi_status);

            if ((midiEvents[i].data[0] & 0xF0) == 0x90)
            {
                arpaggio[note].steps = 0;
                arpaggio[note].note_on = false;
            }

            if ((midiEvents[i].data[0] & 0xF0) == 0x80)
            {
                writeMidiEvent(midiEvents[i]);
                arpaggio[i].steps = fParams[kParamRate];
            }

            printf("%d %d ", midiEvents[i].frame, midiEvents[i].size);
            for (int j = 0; j < 4; j++)
            {
                printf("%02x ", midiEvents[i].data[j]);
            }
            printf("\n");
        }

        if (midiEventCount)
            printf("-------------------\n");

        for (int i = 0; i < 128; i++)
        {

            if ((arpaggio[i].midi_status & 0xF0) == 0x90)
            {
                //printf("steps: %f %f\n", arpaggio[i].steps, ((1.0 - fParams[KParamNoteLen]) * fParams[kParamRate]));

                /* If we are under 0, we run again! */
                if ((arpaggio[i].steps <= 0.0f) && (arpaggio[i].note_on == false))
                {
                   // printf("activate\n");
                    MidiEvent me;
                    memset(&me, 0, sizeof(MidiEvent));
                    me.size = 3;
                    me.data[0] = arpaggio[i].midi_status;
                    me.data[1] = i;
                    me.data[2] = arpaggio[i].midi_velocity;
                    arpaggio[i].steps = (fParams[kParamRate]);
                    writeMidiEvent(me);
                    arpaggio[i].note_on = true;
                    continue;
                }

                arpaggio[i].steps = arpaggio[i].steps - 0.1;
                // printf("note %d on timer = %f\n", i, arpaggio[i].steps);
                /* If we are under node duration, we cut! */

                if ((arpaggio[i].steps <= ((1.0 - fParams[KParamNoteLen]) * fParams[kParamRate])) && (arpaggio[i].note_on == true))
                {
                 //   printf("off\n");
                    MidiEvent me;
                    memset(&me, 0, sizeof(MidiEvent));
                    me.size = 3;
                    me.data[0] = arpaggio[i].midi_status & 0xEF;
                    me.data[1] = i;
                    me.data[2] = arpaggio[i].midi_velocity;
                    writeMidiEvent(me);
                    arpaggio[i].note_on = false;
                }
            }
        }

#endif
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Callbacks (optional)

    /**
       Optional callback to inform the plugin about a sample rate change.@n
       This function will only be called when the plugin is deactivated.
       @see getSampleRate()
     */
    void sampleRateChanged(double newSampleRate) override
    {
        printf("Rate chanted\n");
    }

    // ----------------------------------------------------------------------------------------------------------------

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImGuiPluginDSP)
};

// --------------------------------------------------------------------------------------------------------------------

Plugin *createPlugin()
{
    return new ImGuiPluginDSP();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
