#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace gr;

AmpForgeProcessor::AmpForgeProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout()),
      presets (apvts)
{
    auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    inGainP = raw ("in_gain"); outGainP = raw ("out_gain"); ampOnP = raw ("amp_on"); ampModelP = raw ("amp_model");
    pickupP = raw ("pickup"); osP = raw ("oversample"); gainP = raw ("amp_gain"); volP = raw ("amp_volume");
    bassP = raw ("amp_bass"); midP = raw ("amp_mid"); trebP = raw ("amp_treble"); presP = raw ("amp_presence");
    masterP = raw ("amp_master"); cabOnP = raw ("cab_on"); cabTypeP = raw ("cab_type"); cabSpkP = raw ("cab_speaker");
    cabMicP = raw ("cab_mic"); cabPosP = raw ("cab_micpos"); cabDistP = raw ("cab_micdist"); cabHpP = raw ("cab_hp");
    cabLpP = raw ("cab_lp"); cabLevelP = raw ("cab_level"); cabUserP = raw ("cab_user");

    for (int i = 0; i < kNumPreSlots; ++i) preP[i] = fetchSlot (true, i);
    for (int i = 0; i < kNumPostSlots; ++i) postP[i] = fetchSlot (false, i);

    for (auto* id : { "cab_type", "cab_speaker", "cab_mic", "cab_micpos", "cab_micdist", "cab_user", "oversample" })
        apvts.addParameterListener (id, this);
}

AmpForgeProcessor::~AmpForgeProcessor()
{
    cancelPendingUpdate();
    for (auto* id : { "cab_type", "cab_speaker", "cab_mic", "cab_micpos", "cab_micdist", "cab_user", "oversample" })
        apvts.removeParameterListener (id, this);
}

AmpForgeProcessor::SlotPtrs AmpForgeProcessor::fetchSlot (bool pre, int i)
{
    SlotPtrs s;
    s.type = apvts.getRawParameterValue (slotId (pre, i, "type"));
    s.on = apvts.getRawParameterValue (slotId (pre, i, "on"));
    for (int k = 0; k < 4; ++k) s.p[k] = apvts.getRawParameterValue (slotId (pre, i, "p" + juce::String (k + 1)));
    return s;
}

bool AmpForgeProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return l.getMainInputChannelSet() == out;
}

void AmpForgeProcessor::prepareToPlay (double sr, int block)
{
    currentSampleRate = sr;
    maxBlock = std::max (block, 64);
    for (auto& s : preSlots) s.prepare (sr, maxBlock, 1);
    for (auto& s : postSlots) s.prepare (sr, maxBlock, 2);
    amp.prepare (sr, maxBlock);
    cab.prepare (sr, maxBlock);
    inGain.prepare (sr, 0.02); outGain.prepare (sr, 0.02);
    rebuildCabinet();
    setLatencySamples (amp.getLatencySamples());
}

void AmpForgeProcessor::handleAsyncUpdate()
{
    rebuildCabinet();
    setLatencySamples (amp.getLatencySamples());
}

void AmpForgeProcessor::rebuildCabinet()
{
    if (cabUserP->load() > 0.5f && userIRPath.isNotEmpty() && juce::File (userIRPath).existsAsFile())
        cab.loadFile (juce::File (userIRPath));
    else
        cab.loadBuiltIn ((int) cabTypeP->load(), (int) cabSpkP->load(), (int) cabMicP->load(), cabPosP->load(), cabDistP->load());
}

void AmpForgeProcessor::loadUserIR (const juce::File& f)
{
    userIRPath = f.getFullPathName();
    if (auto* p = apvts.getParameter ("cab_user")) p->setValueNotifyingHost (1.0f);
    triggerAsyncUpdate();
}

void AmpForgeProcessor::swapSlots (bool pre, int a, int b)
{
    const int count = pre ? kNumPreSlots : kNumPostSlots;
    if (a < 0 || b < 0 || a >= count || b >= count || a == b) return;
    for (auto* suffix : { "type", "on", "p1", "p2", "p3", "p4" })
    {
        auto* pa = apvts.getParameter (slotId (pre, a, suffix));
        auto* pb = apvts.getParameter (slotId (pre, b, suffix));
        if (pa == nullptr || pb == nullptr) continue;
        const float va = pa->getValue(), vb = pb->getValue();
        pa->setValueNotifyingHost (vb);
        pb->setValueNotifyingHost (va);
    }
}

void AmpForgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples();
    const int nCh = buffer.getNumChannels();
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c) buffer.clear (c, 0, total);
    if (nCh == 0) return;

    for (int start = 0; start < total; start += maxBlock)
    {
        const int n = std::min (maxBlock, total - start);
        juce::AudioBuffer<float> sub (buffer.getArrayOfWritePointers(), std::min (nCh, 2), start, n);
        processChunk (sub);
    }
}

void AmpForgeProcessor::processChunk (juce::AudioBuffer<float>& buf)
{
    const int n = buf.getNumSamples();
    const int nCh = buf.getNumChannels();
    float* ch0 = buf.getWritePointer (0);
    juce::AudioBuffer<float> mono (&ch0, 1, n); // guitar input = channel 0, processed in place

    // Input gain + meter
    inGain.set (dbToGain (inGainP->load()));
    for (int i = 0; i < n; ++i) ch0[i] *= inGain.next();
    inLevel.store (std::max (mono.getMagnitude (0, n), inLevel.load() * 0.9f));

    float p[4];
    auto runSlot = [&] (EffectSlot& s, const SlotPtrs& sp, juce::AudioBuffer<float>& b)
    {
        for (int k = 0; k < 4; ++k) p[k] = sp.p[k]->load();
        s.process (b, (int) sp.type->load(), sp.on->load() > 0.5f, p);
    };

    // Input -> Pedals -> Amp -> Cabinet
    for (int i = 0; i < kNumPreSlots; ++i) runSlot (preSlots[i], preP[i], mono);

    if (ampOnP->load() > 0.5f)
    {
        AmpModel::Params ap;
        ap.model = (int) ampModelP->load(); ap.pickup = (int) pickupP->load(); ap.oversampling = (int) osP->load();
        ap.gain = gainP->load(); ap.volume = volP->load(); ap.bass = bassP->load(); ap.mid = midP->load();
        ap.treble = trebP->load(); ap.presence = presP->load(); ap.master = masterP->load();
        amp.process (mono, ap);
    }

    CabinetEngine::Params cp;
    cp.on = cabOnP->load() > 0.5f; cp.hp = cabHpP->load(); cp.lp = cabLpP->load(); cp.levelDb = cabLevelP->load();
    cab.process (mono, cp);

    // -> Effects (stereo) -> Output
    if (nCh > 1) buf.copyFrom (1, 0, buf, 0, 0, n);
    for (int i = 0; i < kNumPostSlots; ++i) runSlot (postSlots[i], postP[i], buf);

    outGain.set (dbToGain (outGainP->load()));
    for (int i = 0; i < n; ++i)
    {
        const float g = outGain.next();
        for (int c = 0; c < nCh; ++c)
        {
            float& s = buf.getWritePointer (c)[i];
            s *= g;
            if (! std::isfinite (s)) s = 0.0f;
        }
    }
    outLevel.store (std::max (buf.getMagnitude (0, n), outLevel.load() * 0.9f));
}

void AmpForgeProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("irPath", userIRPath, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void AmpForgeProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto t = juce::ValueTree::fromXml (*xml);
            userIRPath = t.getProperty ("irPath").toString();
            apvts.replaceState (t);
            triggerAsyncUpdate();
        }
}

juce::AudioProcessorEditor* AmpForgeProcessor::createEditor() { return new AmpForgeEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AmpForgeProcessor(); }
