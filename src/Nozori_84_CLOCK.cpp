// --------------------------------------------------------------------------
// This file is part of the NOZORI firmware.
//
//    NOZORI firmware is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    NOZORI firmware is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with NOZORI firmware. If not, see <http://www.gnu.org/licenses/>.
// --------------------------------------------------------------------------

#include "plugin.hpp"

struct Nozori_84_CLOCK : Module {
	enum ParamIds {
		POT2_PARAM,
		POT1_PARAM,
		POT3_PARAM,
		POT4_PARAM,
		POT5_PARAM,
		POT6_PARAM,
		POT7_PARAM,
		POT8_PARAM,
		SWITCH_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN1_INPUT,
		IN2_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT2_OUTPUT,
		OUT1_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		LED4_LIGHT,
		LED2_LIGHT,
		NUM_LIGHTS
	};

	Nozori_84_CLOCK() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(POT2_PARAM, 0.f, 1.f, 0.f, "pot2");
		configParam(POT1_PARAM, 0.f, 1.f, 0.f, "pot1");
		configParam(POT3_PARAM, 0.f, 1.f, 0.f, "pot3");
		configParam(POT4_PARAM, 0.f, 1.f, 0.f, "pot4");
		configParam(POT5_PARAM, 0.f, 1.f, 0.f, "pot5");
		configParam(POT6_PARAM, 0.f, 1.f, 0.f, "pot6");
		configParam(POT7_PARAM, 0.f, 1.f, 0.f, "pot7");
		configParam(POT8_PARAM, 0.f, 1.f, 0.f, "pot8");
		configParam(SWITCH_PARAM, 0.0, 2.0, 2.0, "switch");
	}

    #include "vcv2nozori.hpp"
    #include "a_tables.h"
    #include "c_macro.h"
    #include "c_variables.h"
    #include "c_fonctions.h"
    #include "a_utils.h"
    #include "m84_CLOCK.ino"
    int reduce_data_speed_index;
	int warn_status = 0;
	bool dark = 0;

    void onAdd() override {
        SR_needed = 96000.;
        init_variable();
        init_random();
        CLK_ADSR_init_();
        reduce_data_speed_index = 0;
        CLK_ADSR_loop_(); 
    }

    void onReset() override {
        onAdd();
    }

/*    void onSampleRateChange() override {
        time_since_startup = 0.;
    }
*/
	void process(const ProcessArgs& args) override {
        audio_inL = (uint32_t)((clamp(inputs[IN1_INPUT].getVoltage(), -6.24, 6.24)*322122547.2) + 2147483648.);
        audio_inR = (uint32_t)((clamp(inputs[IN2_INPUT].getVoltage(), -6.24, 6.24)*322122547.2) + 2147483648.);
        reduce_data_speed_index = (reduce_data_speed_index+1)%4;
        if (reduce_data_speed_index == 0) {
            CLK_ADSR_loop_(); // process data loop only at 1/4 the sampling rate in order to be more accurate with the hardware timing
            if (args.sampleRate != SR_needed) {
                if (SR_needed == 96000.) { 
                    warn_status = 96;
                } else if (SR_needed == 48000.) { 
                    warn_status = 48;
                } 
            } else {
                warn_status = 0;
            }
        }
        CLK_ADSR_audio_();
        outputs[OUT1_OUTPUT].setVoltage( ((float)audio_outL - 2147483648.)/322122547.2 );
        outputs[OUT2_OUTPUT].setVoltage( ((float)audio_outR - 2147483648.)/322122547.2 ); 
    }

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "dark", json_boolean(dark));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* darkJ = json_object_get(rootJ, "dark");
		if (darkJ)
			dark = json_boolean_value(darkJ);
	}
};

struct WarningWidget : TransparentWidget {
	int *warn_status = nullptr;
	void draw(const DrawArgs& args) override {
		NVGcolor backgroundColor = nvgRGBA(0xff,0x52,0x5b,0xa0);
		NVGcolor textColor = nvgRGB(0xff, 0xff, 0xff);
		std::string text = "";
		if (visible && warn_status) {
			if(*warn_status == 48) {
				text = "This module needs a 48KHz Sample Rate";
			} else if(*warn_status == 96) {
				text = "This module needs a 96KHz Sample Rate";
			} else {
				return;
			}
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 4.0);
			nvgFillColor(args.vg, backgroundColor);
			nvgFill(args.vg);
			nvgFontSize(args.vg, 9);
			nvgFillColor(args.vg, textColor);
			nvgText(args.vg, 3, 11, text.c_str(), NULL);
		}
	}
};

struct NozoriKnob : RoundKnob {
	NozoriKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/NozoriKnob.svg")));
	}
};

struct Nozori_84_CLOCKWidget : ModuleWidget {
	SvgPanel *panelStandard;
	SvgPanel *panelDark;
	Nozori_84_CLOCKWidget(Nozori_84_CLOCK* module) {
		setModule(module);

		box.size = Vec(12 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
		panelStandard = new SvgPanel();
		panelStandard->box.size = box.size;
		panelStandard->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/nozori_84_CLOCK.svg")));
		panelStandard->show();
		addChild(panelStandard);

		panelDark = new SvgPanel();
		panelDark->box.size = box.size;
		panelDark->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/nozori_84_CLOCK_dark.svg")));
		panelDark->hide();
		addChild(panelDark);

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<NozoriKnob>(mm2px(Vec(46.1, 30.3)), module, Nozori_84_CLOCK::POT2_PARAM));
		addParam(createParamCentered<NozoriKnob>(mm2px(Vec(15.1, 30.3)), module, Nozori_84_CLOCK::POT1_PARAM));
		addParam(createParamCentered<NozoriKnob>(mm2px(Vec(15.1, 52.8)), module, Nozori_84_CLOCK::POT3_PARAM));
		addParam(createParamCentered<NozoriKnob>(mm2px(Vec(46.1, 52.8)), module, Nozori_84_CLOCK::POT4_PARAM));
		addParam(createParamCentered<NozoriKnob>(mm2px(Vec(15.1, 75.1)), module, Nozori_84_CLOCK::POT5_PARAM));
		addParam(createParamCentered<NozoriKnob>(mm2px(Vec(46.1, 75.1)), module, Nozori_84_CLOCK::POT6_PARAM));
		addParam(createParamCentered<NozoriKnob>(mm2px(Vec(15.1, 97.5)), module, Nozori_84_CLOCK::POT7_PARAM));
		addParam(createParamCentered<NozoriKnob>(mm2px(Vec(46.1, 97.5)), module, Nozori_84_CLOCK::POT8_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.9, 113.3)), module, Nozori_84_CLOCK::IN1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.4, 113.3)), module, Nozori_84_CLOCK::IN2_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.3, 113.3)), module, Nozori_84_CLOCK::OUT2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(36.8, 113.3)), module, Nozori_84_CLOCK::OUT1_OUTPUT));

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(58.3, 41.5)), module, Nozori_84_CLOCK::LED4_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(2.9, 41.5)), module, Nozori_84_CLOCK::LED2_LIGHT));

		addParam(createParamCentered<NKK>(mm2px(Vec(30.6, 14.3)), module, Nozori_84_CLOCK::SWITCH_PARAM));

		math::Vec warnSize = Vec(box.size.x - 15, 15);
		WarningWidget* warningDisplay = createWidget<WarningWidget>(Vec(box.size.x/2 - warnSize.x/2, box.size.y - warnSize.y));
		warningDisplay->box.size = warnSize;
		if (module)
   			warningDisplay->warn_status = &module->warn_status;
		#ifndef METAMODULE
		addChild(warningDisplay);
		#endif
	}
	void appendContextMenu(Menu* menu) override {
		Nozori_84_CLOCK* module = dynamic_cast<Nozori_84_CLOCK*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator);

		menu->addChild(createBoolPtrMenuItem("Dark Mode", "", &module->dark));
	}

	void step() override {
		if (module) {
			Nozori_84_CLOCK *module = dynamic_cast<Nozori_84_CLOCK*>(this->module);
			assert(module);
			if (module->dark == 0) {
				panelStandard->show();
				panelDark->hide();
			} else {
				panelStandard->hide();
				panelDark->show();
			}
		}
		ModuleWidget::step();
	}
};

Model* modelNozori_84_CLOCK = createModel<Nozori_84_CLOCK, Nozori_84_CLOCKWidget>("Nozori_84_CLOCK");
