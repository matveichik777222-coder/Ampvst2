#include "PresetManager.h"

juce::File PresetManager::getUserDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("AmpForge").getChildFile ("Presets");
}

void PresetManager::refresh()
{
    entries.clear();
    const auto& fp = getFactoryPresets();
    for (int i = 0; i < (int) fp.size(); ++i)
    {
        Entry e; e.name = fp[(size_t) i].name; e.category = fp[(size_t) i].category;
        e.factory = true; e.factoryIndex = i;
        entries.push_back (e);
    }
    auto dir = getUserDirectory();
    if (dir.isDirectory())
    {
        auto files = dir.findChildFiles (juce::File::findFiles, false, "*.xml");
        files.sort();
        for (auto& f : files)
        {
            Entry e; e.name = f.getFileNameWithoutExtension(); e.category = "User";
            e.factory = false; e.file = f;
            entries.push_back (e);
        }
    }
}

void PresetManager::resetToDefaults()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());
}

void PresetManager::setReal (const juce::String& id, float v)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void PresetManager::load (int index)
{
    if (index < 0 || index >= (int) entries.size()) return;
    const auto& e = entries[(size_t) index];
    if (e.factory)
    {
        resetToDefaults();
        for (auto& kv : getFactoryPresets()[(size_t) e.factoryIndex].values)
            setReal (kv.first, kv.second);
    }
    else if (auto xml = juce::parseXML (e.file))
    {
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

bool PresetManager::saveUser (const juce::String& name)
{
    auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto dir = getUserDirectory();
    if (! dir.createDirectory()) return false;
    auto xml = apvts.copyState().createXml();
    if (xml == nullptr) return false;
    const bool ok = xml->writeTo (dir.getChildFile (clean + ".xml"));
    refresh();
    return ok;
}
