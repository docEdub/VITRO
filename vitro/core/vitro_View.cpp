namespace vitro {

static void copyElementAttributesFromXml(const Element::Ptr& element, const XmlElement& xmlElement)
{
    for (int i = 0; i < xmlElement.getNumAttributes(); ++i) {
        const auto& name{ xmlElement.getAttributeName(i) };
        const auto& value{ xmlElement.getAttributeValue(i) };

        element->setAttribute(name, value);
    }
}

Element::Ptr createElementFromXml(Context& ctx, const XmlElement& xmlElement);

static void populateChildElementsFromXml(Context& ctx, const Element::Ptr& element, const XmlElement& xmlElement)
{
    jassert(element != nullptr);

    for (auto* child : xmlElement.getChildIterator()) {
        copyElementAttributesFromXml(element, xmlElement);

        if (auto childElement{ createElementFromXml(ctx, *child) })
            element->addChildElement(childElement);
    }
}

static Element::Ptr createElementFromXml(Context& ctx, const XmlElement& xmlElement)
{
    if (xmlElement.isTextElement())
        return nullptr;

    const Identifier tag{ xmlElement.getTagName() };

    auto element{ ctx.getElementsFactory().createElement(tag) };

    if (element != nullptr)
        populateChildElementsFromXml(ctx, element, xmlElement);

    return element;
}

//==============================================================================

const Identifier View::tag("View");

View::View(Context& ctx)
    : ComponentElement(View::tag, ctx)
{
}

View::~View()
{
    // We must remove all the children in order for them to be notified
    // that they are about to be deleted. This is important for some
    // element to release resources and perform cleaning up.
    removeAllChildElements();
}

void View::populateFromXml(const XmlElement& xmlElement)
{
    removeAllChildElements();

    const Identifier tag{ xmlElement.getTagName() };

    if (tag != View::tag)
        return;

    populateChildElementsFromXml(context, shared_from_this(), xmlElement);
}

void View::populateFromXmlString(const String& xmlString)
{
    if (auto xml{ XmlDocument::parse(xmlString) })
        populateFromXml(*xml);
    else
        removeAllChildElements();
}

void View::populateFromXmlResource(const String& location)
{
    if (auto xml{ context.getLoader().loadXML(location) })
        populateFromXml(*xml);
    else
        removeAllChildElements();
}

void View::resized()
{
    recalculateLayoutToCurrentBounds();
}

void View::update()
{
    triggerAsyncUpdate();
}

void View::handleAsyncUpdate()
{
    updateEverything();
}

void View::updateEverything()
{
    if (updateLayout())
        recalculateLayoutToCurrentBounds();

    updateChildren();

    repaint();
}

void View::recalculateLayoutToCurrentBounds()
{
    recalculateLayout(static_cast<float>(getWidth()), static_cast<float>(getHeight()));
}

} // namespace vitro