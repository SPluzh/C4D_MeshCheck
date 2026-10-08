#ifndef MESHCHECK_TAG_H__
#define MESHCHECK_TAG_H__

#include "c4d.h"
#include "description/tmeshcheck.h"

#define PLUGIN_ID_MESHCHECK_TAG     1067830
#define PLUGIN_ID_MESHCHECK_COMMAND 1067831

namespace cinema
{

struct HighlightEdge
{
    Int32 v0 = NOTOK;
    Int32 v1 = NOTOK;
    Float angle = 0.0;
    Bool  isBoundary = false;
    Bool  isNonManifold = false;
    Bool  isAngleProblem = false;
    Bool  isHardEdge = false;
    Bool  isUVSeam = false;
};

class MeshCheckTagData : public TagData
{
public:
    virtual Bool Init(GeListNode* node, Bool isCloneInit) override;
    virtual Bool GetDDescription(const GeListNode* node, Description* description, DESCFLAGS_DESC& flags) const override;
    virtual Bool GetDEnabling(const GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) const override;
    virtual EXECUTIONRESULT Execute(BaseTag* tag, BaseDocument* doc, BaseObject* op, BaseThread* bt, Int32 priority, EXECUTIONFLAGS flags) override;
    virtual Bool Draw(BaseTag* tag, BaseObject* op, BaseDraw* bd, BaseDrawHelp* bh) override;
    virtual Bool Message(GeListNode* node, Int32 type, void* data) override;

    static NodeData* Alloc() { return NewObjClear(MeshCheckTagData); }

private:
    void UpdateMeshEdges(BaseTag* tag, BaseObject* op, Bool forceRecalculate) const;
    const PolygonObject* GetPolygonObject(BaseObject* op) const;
    static Vector CalcPolyNormal(const Vector* points, const CPolygon& poly);

    // Cached problematic edges for instant viewport drawing
    mutable maxon::BaseArray<HighlightEdge> m_highlightEdges;
    mutable UInt64 m_lastDirtyChecksum = 0;
    mutable Int32  m_lastPointCount = 0;
    mutable Int32  m_lastPolyCount = 0;
    mutable Float  m_lastThreshold = -1.0;
    mutable Bool   m_lastShowAngle = true;
    mutable Bool   m_lastAngleIgnoreHard = false;
    mutable Bool   m_lastShowHardEdges = false;
    mutable Bool   m_lastShowUVSeams = false;
    mutable Bool   m_lastShowBoundary = false;
    mutable Bool   m_lastEnabled = true;
    mutable Int32  m_problemEdgeCount = 0;
    mutable Int32  m_boundaryEdgeCount = 0;
    mutable Int32  m_hardEdgeCount = 0;
    mutable Int32  m_uvSeamCount = 0;
};

class MeshCheckCommand : public CommandData
{
public:
    virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager) override;
};

Bool RegisterMeshCheckTag();
Bool RegisterMeshCheckCommand();

} // namespace cinema

#endif // MESHCHECK_TAG_H__
