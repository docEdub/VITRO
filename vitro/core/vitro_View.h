namespace vitro {

/** The top-most view container.

    The view must be the top-most element of the UI hierarchy.
*/

class View : public ComponentElement,
             public juce::Component,
             private juce::AsyncUpdater
{
public:

    const static juce::Identifier tag;  // <View>

    static JSClassID jsClassID;

    View(Context& ctx);
    ~View();

    juce::Component* getComponent() override { return this; }

    void populateFromXml(const juce::XmlElement& xmlElement);

    void populateFromXmlString(const juce::String& xmlString);

    void populateFromXmlResource(const juce::String& location);


    // juce::Component
    void resized() override;

protected:

    // vitro::Element
    void update() override;

private:

    // juce::AsyncUpdater
    void handleAsyncUpdate() override;

    void updateEverything();

    void recalculateLayoutToCurrentBounds();
};

} // namespace vitro
