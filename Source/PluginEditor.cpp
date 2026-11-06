#include "PluginEditor.h"

AmpForgeEditor::AmpForgeEditor (AmpForgeProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      inMeter (p.inLevel, "IN"), outMeter (p.outLevel, "OUT"),
      pedals (p, true, gr::kNumPreSlots), effects (p, false, gr::kNumPostSlots),
      ampPage (p), cabPage (p)
{
    setLookAndFeel (&laf);
    setSize (980, 600);

    pages[0] = &pedals; pages[1] = &ampPage; pages[2] = &cabPage; pages[3] = &effects;
    const char* names[4] = { "PEDALS", "AMP", "CABINET", "EFFECTS" };
    for (int i = 0; i < 4; ++i)
    {
        tabs[i].setButtonText (names[i]);
        tabs[i].setClickingTogglesState (true);
        tabs[i].setRadioGroupId (1001);
        tabs[i].onClick = [this, i] { showPage (i); };
        addAndMakeVisible (tabs[i]);
        addChildComponent (pages[i]);
    }
    for (int i = 0; i < 4; ++i) addChildComponent (*pages[i]);

    rebuildPresetBox();
    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.onChange = [this] { proc.presets.load (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (1); };
    saveBtn.onClick = [this] { promptSave(); };
    addAndMakeVisible (prevBtn); addAndMakeVisible (nextBtn); addAndMakeVisible (saveBtn);

    inKnob = std::make_unique<ui::Knob> (proc.apvts, "in_gain", "INPUT", ui::Knob::Fmt::Db);
    outKnob = std::make_unique<ui::Knob> (proc.apvts, "out_gain", "OUTPUT", ui::Knob::Fmt::Db);
    addAndMakeVisible (*inKnob); addAndMakeVisible (*outKnob);
    addAndMakeVisible (inMeter); addAndMakeVisible (outMeter);

    tabs[1].setToggleState (true, juce::dontSendNotification);
    showPage (1);
}

AmpForgeEditor::~AmpForgeEditor() { setLookAndFeel (nullptr); }

void AmpForgeEditor::showPage (int i)
{
    for (int k = 0; k < 4; ++k) pages[k]->setVisible (k == i);
}

void AmpForgeEditor::rebuildPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    juce::String lastCat;
    const auto& es = proc.presets.getEntries();
    for (int i = 0; i < (int) es.size(); ++i)
    {
        if (es[(size_t) i].category != lastCat)
        {
            lastCat = es[(size_t) i].category;
            if (i > 0) presetBox.addSeparator();
            presetBox.addSectionHeading (lastCat);
        }
        presetBox.addItem (es[(size_t) i].name, i + 1);
    }
}

void AmpForgeEditor::stepPreset (int delta)
{
    const int n = (int) proc.presets.getEntries().size();
    if (n == 0) return;
    int id = presetBox.getSelectedId();
    id = id <= 0 ? (delta > 0 ? 1 : n) : ((id - 1 + delta + n) % n) + 1;
    presetBox.setSelectedId (id, juce::sendNotification);
}

void AmpForgeEditor::promptSave()
{
    auto* aw = new juce::AlertWindow ("Save preset", "Preset name:", juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor ("name", "My Preset", "");
    aw->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<AmpForgeEditor> safe (this);
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([safe, aw] (int result)
    {
        if (result != 1 || safe == nullptr) return;
        const auto name = aw->getTextEditorContents ("name");
        if (safe->proc.presets.saveUser (name))
        {
            safe->rebuildPresetBox();
            const auto& es = safe->proc.presets.getEntries();
            for (int i = 0; i < (int) es.size(); ++i)
                if (! es[(size_t) i].factory && es[(size_t) i].name == juce::File::createLegalFileName (name.trim()))
                    safe->presetBox.setSelectedId (i + 1, juce::dontSendNotification);
        }
    }), true);
}

void AmpForgeEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff111214));
    g.setColour (juce::Colour (0xff17181b));
    g.fillRect (0, 0, getWidth(), 92);
    g.setColour (ui::accent());
    g.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    g.drawText ("AMPFORGE", 20, 10, 220, 34, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff2a2b30));
    g.drawHorizontalLine (92, 0.0f, (float) getWidth());
}

void AmpForgeEditor::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (92);
    auto topInner = top.reduced (20, 8);

    auto tabRow = topInner.removeFromBottom (30);
    for (int i = 0; i < 4; ++i) { tabs[i].setBounds (tabRow.removeFromLeft (110)); tabRow.removeFromLeft (6); }

    auto right = topInner.removeFromRight (300);
    outMeter.setBounds (right.removeFromRight (70).withTrimmedBottom (10).withTrimmedTop (4)); right.removeFromRight (6);
    inMeter.setBounds (right.removeFromRight (70).withTrimmedBottom (10).withTrimmedTop (4));
    // input/output trim knobs live in the preset bar row
    inKnob->setBounds (right.removeFromLeft (70).withHeight (88).withY (2));
    outKnob->setBounds (right.removeFromLeft (70).withHeight (88).withY (2));

    auto presetRow = topInner.removeFromTop (30).withTrimmedLeft (240);
    prevBtn.setBounds (presetRow.removeFromLeft (30)); presetRow.removeFromLeft (4);
    saveBtn.setBounds (presetRow.removeFromRight (60)); presetRow.removeFromRight (4);
    nextBtn.setBounds (presetRow.removeFromRight (30)); presetRow.removeFromRight (4);
    presetBox.setBounds (presetRow);

    for (auto* pg : pages) pg->setBounds (r.reduced (10));
}
