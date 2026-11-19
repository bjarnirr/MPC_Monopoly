#include "mpc_plugin.h"
#include "MonoPolyEngine.h"
#include <algorithm>
#include <initializer_list>
#include <utility>
#include <vector>

class MonoPolyPlugin final : public mpc::Plugin
{
public:
    enum P {
        kVoiceMode,
        kW1,kO1,kT1,kL1,kPW1,
        kW2,kO2,kT2,kL2,kPW2,
        kW3,kO3,kT3,kL3,kPW3,
        kW4,kO4,kT4,kL4,kPW4,
        kNoise,kCutoff,kReso,kFEnvAmt,kKeyTrack,
        kFA,kFD,kFS,kFR,
        kAA,kAD,kAS,kAR,
        kMG1Shape,kMG1Rate,kMG2Shape,kMG2Rate,
        kPitchMod,kFilterMod,kPWMMod,kMaster,
        kCount
    };

    static mpc::PluginInfo info() {
        mpc::PluginInfo i;
        i.name = "HolyPoly";
        i.vendor = "HolyPoly";
        i.uniqueId = MPC_FOURCC('H','P','l','y');
        i.version = 100;
        i.isInstrument = true;
        i.numInputs = 0;
        i.numOutputs = 2;
        return i;
    }

    MonoPolyPlugin() : mpc::Plugin(info())
    {
        using namespace mpc;

        addParam(choiceParam("voice_mode", "Voice Mode", {"Mono", "2 Voice", "4 Voice"}, 0));

        const std::vector<std::string> waves = {"Triangle", "Saw", "Pulse", "PWM"};
        const char* waveIds[4] = {"vco1_wave", "vco2_wave", "vco3_wave", "vco4_wave"};
        const char* waveNames[4] = {"VCO1 Wave", "VCO2 Wave", "VCO3 Wave", "VCO4 Wave"};
        const char* octIds[4] = {"vco1_oct", "vco2_oct", "vco3_oct", "vco4_oct"};
        const char* octNames[4] = {"VCO1 Oct", "VCO2 Oct", "VCO3 Oct", "VCO4 Oct"};
        const char* tuneIds[4] = {"vco1_tune", "vco2_tune", "vco3_tune", "vco4_tune"};
        const char* tuneNames[4] = {"VCO1 Tune", "VCO2 Tune", "VCO3 Tune", "VCO4 Tune"};
        const char* levelIds[4] = {"vco1_level", "vco2_level", "vco3_level", "vco4_level"};
        const char* levelNames[4] = {"VCO1 Level", "VCO2 Level", "VCO3 Level", "VCO4 Level"};
        const char* pwIds[4] = {"vco1_pw", "vco2_pw", "vco3_pw", "vco4_pw"};
        const char* pwNames[4] = {"VCO1 PulseW", "VCO2 PulseW", "VCO3 PulseW", "VCO4 PulseW"};

        for (int n = 0; n < 4; ++n) {
            addParam(choiceParam(waveIds[n], waveNames[n], waves, 1));
            addParam(intParam(octIds[n], octNames[n], -2, 1, 0, "oct"));
            addParam(floatParam(tuneIds[n], tuneNames[n], -100, 100, 0, "ct", 1));
            addParam(floatParam(levelIds[n], levelNames[n], 0, 100, n == 0 ? 75 : 55, "%", 0));
            addParam(floatParam(pwIds[n], pwNames[n], 5, 95, 50, "%", 0));
        }

        addParam(floatParam("noise", "Noise", 0, 100, 0, "%", 0));
        addParam(logParam("cutoff", "Cutoff", 20, 18000, 9000, "Hz", 0));
        addParam(floatParam("resonance", "Resonance", 0, 95, 15, "%", 0));
        addParam(floatParam("vcf_env", "VCF Env", 0, 100, 25, "%", 0));
        addParam(floatParam("key_track", "Key Track", 0, 100, 0, "%", 0));
        addParam(logParam("vcf_attack", "VCF Attack", 1, 5000, 10, "ms", 0));
        addParam(logParam("vcf_decay", "VCF Decay", 1, 5000, 300, "ms", 0));
        addParam(floatParam("vcf_sustain", "VCF Sustain", 0, 100, 0, "%", 0));
        addParam(logParam("vcf_release", "VCF Release", 1, 5000, 300, "ms", 0));
        addParam(logParam("vca_attack", "VCA Attack", 1, 5000, 10, "ms", 0));
        addParam(logParam("vca_decay", "VCA Decay", 1, 5000, 250, "ms", 0));
        addParam(floatParam("vca_sustain", "VCA Sustain", 0, 100, 75, "%", 0));
        addParam(logParam("vca_release", "VCA Release", 1, 5000, 400, "ms", 0));
        addParam(choiceParam("mg1_shape", "MG1 Shape", {"Sine", "Triangle", "Ramp", "Square"}, 1));
        addParam(logParam("mg1_rate", "MG1 Rate", 0.05f, 30, 5, "Hz", 2));
        addParam(choiceParam("mg2_shape", "MG2 Shape", {"Sine", "Triangle", "Ramp", "Square"}, 0));
        addParam(logParam("mg2_rate", "MG2 Rate", 0.05f, 30, 0.7f, "Hz", 2));
        addParam(floatParam("pitch_mod", "Pitch Mod", 0, 100, 0, "%", 0));
        addParam(floatParam("filter_mod", "Filter Mod", 0, 100, 0, "%", 0));
        addParam(floatParam("pwm_mod", "PWM Mod", 0, 100, 0, "%", 0));
        addParam(floatParam("master", "Master", 0, 100, 70, "%", 0));

        auto preset = [this](const char* name, std::initializer_list<std::pair<int, float>> changes) {
            std::vector<float> v;
            v.reserve(numParams());
            for (int i = 0; i < numParams(); ++i) v.push_back(param(i).def);
            for (auto c : changes) v[(size_t)c.first] = c.second;
            addProgram(name, std::move(v));
        };

        preset("Init", {});
        preset("Holy Bass", {{kVoiceMode,0},{kCutoff,850},{kReso,38},{kFEnvAmt,70},{kFA,2},{kFD,180},{kAA,2},{kAD,220},{kAS,65},{kAR,130},{kL1,90},{kL2,70},{kO2,-1},{kT2,8}});
        preset("Poly Brass", {{kVoiceMode,2},{kCutoff,2400},{kReso,26},{kFEnvAmt,55},{kFA,8},{kFD,190},{kAA,6},{kAD,260},{kAS,52},{kAR,420},{kL1,82},{kL2,68},{kL3,74},{kL4,52},{kT1,6},{kT2,3}});
        preset("Analog Pad", {{kVoiceMode,4},{kCutoff,1200},{kReso,18},{kFEnvAmt,52},{kFA,18},{kFD,420},{kAS,88},{kAR,580},{kL1,72},{kL2,60},{kL3,58},{kL4,62},{kMG1Rate,0.9f},{kPitchMod,28}});
        preset("PWM Sweep", {{kVoiceMode,2},{kW1,3},{kW2,3},{kW3,3},{kW4,3},{kPWMMod,55},{kMG2Rate,0.35f},{kCutoff,5000},{kAS,90},{kAR,900}});
        preset("Chime Lead", {{kVoiceMode,1},{kCutoff,3600},{kReso,12},{kFEnvAmt,68},{kFA,2},{kFD,120},{kAA,2},{kAD,160},{kAS,28},{kAR,240},{kW1,0},{kT1,8},{kL1,86},{kPW1,56}});
        preset("Low Drones", {{kVoiceMode,4},{kCutoff,480},{kReso,42},{kFEnvAmt,84},{kFA,20},{kFD,600},{kAS,72},{kAR,700},{kL1,75},{kL2,66},{kL3,60},{kL4,58},{kNoise,20},{kMaster,58}});
        preset("Mellow Choir", {{kVoiceMode,4},{kCutoff,1700},{kReso,20},{kFEnvAmt,35},{kFA,12},{kFD,300},{kAS,82},{kAR,520},{kL1,78},{kL2,70},{kL3,60},{kL4,55},{kMG1Rate,0.6f},{kPitchMod,18}});
        preset("Mono Hero", {{kVoiceMode,0},{kCutoff,2200},{kReso,30},{kFEnvAmt,80},{kFA,3},{kFD,150},{kAA,2},{kAD,150},{kAS,45},{kAR,220},{kL1,92},{kO1,0},{kT1,12},{kW1,1},{kPWMMod,32}});

        preset("Glass Pluck", {{kVoiceMode,1},{kW1,0},{kW2,3},{kW3,1},{kCutoff,4200},{kReso,48},{kFEnvAmt,72},{kFA,1},{kFD,90},{kFS,15},{kFR,180},{kAA,2},{kAD,280},{kAS,30},{kAR,260},{kT2,7},{kPitchMod,12}});
        preset("Rubber Sequence", {{kVoiceMode,0},{kW1,2},{kW2,1},{kO2,-1},{kT2,-5},{kCutoff,620},{kReso,62},{kFEnvAmt,82},{kFA,2},{kFD,140},{kFS,8},{kFR,120},{kAA,2},{kAD,180},{kAS,45},{kAR,110},{kFilterMod,46},{kMG1Shape,3},{kMG1Rate,8.0f}});
        preset("Soft Organ", {{kVoiceMode,4},{kW1,0},{kW2,1},{kW3,0},{kW4,1},{kO1,0},{kO2,0},{kO3,-1},{kT2,12},{kCutoff,2800},{kReso,14},{kFEnvAmt,20},{kFA,12},{kFD,220},{kFS,70},{kFR,400},{kAA,8},{kAD,200},{kAS,82},{kAR,350},{kL1,90},{kL2,52},{kL3,70},{kL4,40}});
        preset("Dark Arp", {{kVoiceMode,1},{kW1,1},{kW2,2},{kO2,-1},{kCutoff,780},{kReso,54},{kFEnvAmt,78},{kFA,4},{kFD,220},{kFS,10},{kFR,260},{kAA,2},{kAD,140},{kAS,40},{kAR,180},{kL1,88},{kL2,64},{kFilterMod,58},{kMG2Shape,2},{kMG2Rate,2.5f}});
        preset("Lunar Bell", {{kVoiceMode,2},{kW1,0},{kW2,0},{kW3,1},{kT1,12},{kT2,-12},{kT3,19},{kCutoff,5600},{kReso,32},{kFEnvAmt,36},{kFA,8},{kFD,520},{kFS,0},{kFR,900},{kAA,3},{kAD,920},{kAS,18},{kAR,1100},{kL1,82},{kL2,48},{kL3,36},{kMG1Shape,0},{kMG1Rate,0.28f},{kPitchMod,22}});
        preset("Electric Keys", {{kVoiceMode,2},{kW1,1},{kW2,3},{kO2,0},{kCutoff,3000},{kReso,28},{kFEnvAmt,65},{kFA,3},{kFD,480},{kFS,18},{kFR,520},{kAA,2},{kAD,640},{kAS,46},{kAR,500},{kL1,76},{kL2,58},{kL3,36},{kT2,-7},{kKeyTrack,62}});
        preset("Warm Stack", {{kVoiceMode,4},{kW1,1},{kW2,1},{kW3,0},{kO2,-1},{kO3,-1},{kT1,-5},{kT2,5},{kT3,7},{kCutoff,2100},{kReso,22},{kFEnvAmt,44},{kFA,20},{kFD,350},{kFS,72},{kFR,650},{kAA,12},{kAD,260},{kAS,86},{kAR,720},{kL1,82},{kL2,70},{kL3,55},{kL4,48},{kMG1Shape,1},{kMG1Rate,0.18f},{kPitchMod,8}});
        preset("Sub Current", {{kVoiceMode,0},{kW1,1},{kW2,1},{kO1,-1},{kO2,-1},{kT2,-1},{kCutoff,260},{kReso,72},{kFEnvAmt,90},{kFA,5},{kFD,300},{kFS,24},{kFR,220},{kAA,4},{kAD,380},{kAS,70},{kAR,260},{kL1,100},{kL2,48},{kNoise,8},{kMaster,82}});
        preset("Silver Lead", {{kVoiceMode,0},{kW1,1},{kW2,2},{kT1,5},{kT2,-5},{kCutoff,3900},{kReso,38},{kFEnvAmt,74},{kFA,2},{kFD,180},{kFS,12},{kFR,300},{kAA,3},{kAD,260},{kAS,70},{kAR,300},{kL1,88},{kL2,64},{kPWMMod,42},{kMG2Shape,0},{kMG2Rate,4.8f},{kPitchMod,6}});
        preset("Cloud Choir", {{kVoiceMode,4},{kW1,0},{kW2,0},{kW3,1},{kW4,2},{kCutoff,1550},{kReso,16},{kFEnvAmt,28},{kFA,45},{kFD,700},{kFS,82},{kFR,1400},{kAA,40},{kAD,500},{kAS,92},{kAR,1500},{kL1,68},{kL2,62},{kL3,52},{kL4,44},{kMG1Shape,0},{kMG1Rate,0.11f},{kPitchMod,24},{kMaster,62}});
        preset("Neon Pluck", {{kVoiceMode,1},{kW1,3},{kW2,3},{kT1,12},{kT2,-12},{kCutoff,6800},{kReso,44},{kFEnvAmt,88},{kFA,1},{kFD,70},{kFS,5},{kFR,150},{kAA,1},{kAD,170},{kAS,22},{kAR,150},{kL1,86},{kL2,54},{kPWMMod,68},{kMG2Rate,0.8f}});
        preset("Fifth Space", {{kVoiceMode,4},{kW1,1},{kW2,1},{kW3,2},{kW4,0},{kO2,0},{kO3,0},{kT1,-7},{kT2,7},{kT3,-12},{kCutoff,980},{kReso,30},{kFEnvAmt,42},{kFA,65},{kFD,950},{kFS,68},{kFR,1800},{kAA,50},{kAD,800},{kAS,76},{kAR,1800},{kL1,72},{kL2,58},{kL3,48},{kL4,42},{kMG1Shape,1},{kMG1Rate,0.22f},{kPitchMod,16},{kMaster,64}});
        preset("Tape Pulse", {{kVoiceMode,2},{kW1,2},{kW2,1},{kW3,3},{kT2,-5},{kCutoff,3400},{kReso,24},{kFEnvAmt,50},{kFA,10},{kFD,260},{kFS,26},{kFR,380},{kAA,8},{kAD,310},{kAS,64},{kAR,420},{kL1,74},{kL2,62},{kL3,48},{kL4,36},{kNoise,12},{kMG2Shape,3},{kMG2Rate,1.2f},{kPWMMod,24},{kFilterMod,18}});
        preset("Metro Bass", {{kVoiceMode,0},{kW1,2},{kW2,1},{kO2,-1},{kT2,3},{kPW1,28},{kPW2,64},{kCutoff,720},{kReso,58},{kFEnvAmt,92},{kFA,2},{kFD,200},{kFS,12},{kFR,160},{kAA,2},{kAD,260},{kAS,56},{kAR,140},{kL1,92},{kL2,66},{kFilterMod,42},{kMG1Shape,3},{kMG1Rate,6.0f}});
        preset("Velvet Pad", {{kVoiceMode,4},{kW1,0},{kW2,1},{kW3,0},{kW4,1},{kT1,-4},{kT2,4},{kT3,-9},{kT4,9},{kCutoff,1320},{kReso,20},{kFEnvAmt,38},{kFA,55},{kFD,850},{kFS,78},{kFR,1600},{kAA,48},{kAD,700},{kAS,88},{kAR,1500},{kL1,70},{kL2,64},{kL3,58},{kL4,52},{kMG1Shape,0},{kMG1Rate,0.14f},{kPitchMod,12},{kMaster,68}});
    }

    void prepare(double sr, int) override { engine_.prepare(sr); }
    void reset() override { engine_.reset(); }

    void process(mpc::ProcessContext& ctx) override
    {
        monopoly::Parameters p;
        p.voiceMode = getInt(kVoiceMode) == 0 ? monopoly::VoiceMode::Mono : (getInt(kVoiceMode) == 1 ? monopoly::VoiceMode::Two : monopoly::VoiceMode::Four);
        for (int n = 0; n < 4; ++n) {
            const int base = 1 + n * 5;
            p.osc[n].waveform = static_cast<monopoly::Waveform>(std::max(0, std::min(3, getInt(base))));
            p.osc[n].octave = getInt(base + 1);
            p.osc[n].tuneCents = get(base + 2);
            p.osc[n].level = get(base + 3) * 0.01f;
            p.osc[n].pulseWidth = get(base + 4) * 0.01f;
        }
        p.noiseLevel = get(kNoise) * 0.01f;
        p.cutoffHz = get(kCutoff);
        p.resonance = get(kReso) * 0.01f;
        p.filterEnvAmount = get(kFEnvAmt) * 0.01f;
        p.filterKeyTrack = get(kKeyTrack) * 0.01f;
        p.filterEnv = {get(kFA) * 0.001f, get(kFD) * 0.001f, get(kFS) * 0.01f, get(kFR) * 0.001f};
        p.ampEnv = {get(kAA) * 0.001f, get(kAD) * 0.001f, get(kAS) * 0.01f, get(kAR) * 0.001f};
        p.mg1Wave = static_cast<monopoly::LfoWave>(getInt(kMG1Shape));
        p.mg1RateHz = get(kMG1Rate);
        p.mg2Wave = static_cast<monopoly::LfoWave>(getInt(kMG2Shape));
        p.mg2RateHz = get(kMG2Rate);
        p.pitchModDepth = get(kPitchMod) * 0.01f;
        p.filterModDepth = get(kFilterMod) * 0.01f;
        p.pwmModDepth = get(kPWMMod) * 0.01f;
        p.masterLevel = get(kMaster) * 0.01f;

        float* L = ctx.outputs[0];
        float* R = ctx.numOutputs > 1 ? ctx.outputs[1] : nullptr;
        int pos = 0;
        for (int e = 0; e < ctx.numMidiEvents; ++e) {
            const auto& ev = ctx.midi[e];
            const int at = std::max(pos, std::min(ctx.numFrames, ev.sampleOffset));
            if (at > pos) {
                engine_.process(L + pos, R ? R + pos : nullptr, at - pos, p);
                pos = at;
            }
            if (ev.isNoteOn()) engine_.noteOn(ev.note(), ev.velocity() / 127.0f);
            else if (ev.isNoteOff()) engine_.noteOff(ev.note());
            else if (ev.isAllNotesOff()) engine_.allNotesOff();
        }
        if (pos < ctx.numFrames) engine_.process(L + pos, R ? R + pos : nullptr, ctx.numFrames - pos, p);
    }

private:
    monopoly::MonoPolyEngine engine_;
};

MPC_PLUGIN_ENTRY(MonoPolyPlugin)

// Source remains original: all patches above are independently designed for HolyPoly.
