#ifndef COMPONENT_HPP
#define COMPONENT_HPP

// =============================================================================
// OpenCK Component-Property Architecture
// =============================================================================
//
// This file is the abstract base for the *form component* system that
// powers the record editor. The pattern is borrowed (clean-room) from
// the real Starfield Creation Kit's "FormComponent + EditorProperty"
// design:
//
//   TESForm (record)
//      |
//      +-- TESModel_Component
//      |     +-- BGSStringEditorProperty("Model File Name")
//      |     \-- BGSStringEditorProperty("LOD File Name")
//      +-- TESHealth_Component
//      |     \-- BGSFloatEditorProperty("Health")
//      \-- BGSKeywordForm_Component
//            \-- BGSFormArrayEditorProperty("Keywords")
//
// Each Component knows how to:
//   * read and write its slice of the record's subrecords
//   * produce the list of EditorProperty leaves that expose its data
//   * clone, copy, and merge with another component of the same type
//
// The editor UI walks `record->components()` at runtime, so adding a
// new record type is just "compose these components" — no bespoke
// dialog code.
//
// See docs/CK_Real_Integration_Plan.md for the architectural rationale
// and how it maps to the real CK's source tree.

#include <QString>
#include <memory>
#include <vector>
#include "common.hpp"
#include "esmwriter.hpp"

class ESMReader;
class ESMWriter;
class EditorProperty;

class Component
{
public:
    virtual ~Component() = default;

    // The display name shown in the editor's property grid section
    // header. Should be a short human-readable label, not a file-path
    // class name — e.g. "Model", "Health", "Keywords". The CK's
    // equivalent comes from BGSComponent::GetComponentName() but is
    // not user-customizable in Tier 1+2.
    virtual QString name() const = 0;

    // Unique, stable identifier used for serialization compatibility
    // and the registry. Use the same name as the CK's class name
    // (e.g. "TESModel", "BGSKeywordForm") so debugging against
    // real CK output is straightforward, but this is an
    // internal-only string — never shown in the UI.
    virtual QString className() const = 0;

    // Reads the component's subrecords from the current record
    // position. Components are responsible for the entire subrecord
    // block they own — including unknown trailing subrecords, which
    // they should preserve verbatim in rawSubRecords so a future
    // load+save round-trip is lossless.
    virtual void load(ESMReader& esm) = 0;

    // Optional subrecord handling for records that dispatch subrecords individually
    virtual bool canHandle(NAME /*subrecordName*/) const { return false; }
    virtual void handleSubrecord(ESMReader& /*esm*/, NAME /*subrecordName*/) {}
    virtual void handleSubrecord(NAME subrecordName, ESMReader& esm) { handleSubrecord(esm, subrecordName); }

    // Writes the component's subrecords in canonical order. Unknown
    // subrecords preserved from load() must be emitted back out.
    virtual void save(ESMWriter& esm) const = 0;

    // Builds the list of EditorProperty leaves that the property
    // grid will render. Each call returns a fresh list; the grid
    // takes ownership via unique_ptr.
    virtual std::vector<std::unique_ptr<EditorProperty>>
        createEditorProperties() = 0;

    // Returns a deep copy of this component, including any raw
    // subrecords and sub-data the concrete subclass owns.
    virtual std::unique_ptr<Component> clone() const = 0;

    // Copies the data of *other* into this component, assuming
    // both are the same className(). Caller is responsible for
    // verifying the className matches before calling.
    virtual void copyFrom(const Component* other) = 0;

    // Returns true if this component's data is structurally equal
    // to *other*'s, assuming both are the same className().
    virtual bool isEqualTo(const Component* other) const = 0;

    // Merges the data of *other* into this component using the
    // conflict resolution rules appropriate for this component
    // type. For most simple components this is just copyFrom();
    // for arrays (keywords, container items) it appends.
    virtual void mergeWith(const Component* other) = 0;
};

#endif // COMPONENT_HPP
