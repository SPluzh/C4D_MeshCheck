#include "meshcheck_tag.h"
#include "c4d_symbols.h"
#include "description/tmeshcheck.h"

namespace cinema
{

struct EdgePolys
{
    Int32 poly0 = NOTOK;
    Int32 poly1 = NOTOK;
    Int32 polyCount = 0;
};

Bool MeshCheckTagData::Init(GeListNode* node, Bool isCloneInit)
{
    BaseTag* tag = static_cast<BaseTag*>(node);
    if (!tag) return false;
    BaseContainer* data = tag->GetDataInstance();
    if (!data) return false;

    if (!isCloneInit)
    {
        data->SetBool(MESHCHECK_ENABLED, true);
        data->SetFloat(MESHCHECK_ANGLE_THRESHOLD, 89.0);
        data->SetFloat(MESHCHECK_EDGE_WIDTH, 2.5);
        data->SetBool(MESHCHECK_DEPTH_TEST, true);
        data->SetBool(MESHCHECK_USE_GRADIENT, true);
        data->SetVector(MESHCHECK_EDGE_COLOR, Vector(1.0, 0.2, 0.0));
        data->SetVector(MESHCHECK_COLOR_MIN, Vector(1.0, 0.85, 0.0));
        data->SetVector(MESHCHECK_COLOR_MAX, Vector(1.0, 0.05, 0.05));
        data->SetBool(MESHCHECK_SHOW_BOUNDARY, false);
        data->SetVector(MESHCHECK_BOUNDARY_COLOR, Vector(0.0, 0.75, 1.0));
        data->SetString(MESHCHECK_INFO_COUNT, "Ready"_s);
    }

    return true;
}

Bool MeshCheckTagData::GetDDescription(const GeListNode* node, Description* description, DESCFLAGS_DESC& flags) const
{
    if (!description)
        return false;

    if (!description->LoadDescription("Tmeshcheck"_s) && !description->LoadDescription(PLUGIN_ID_MESHCHECK_TAG))
    {
        // Description not preloaded from disk; continue
    }

    flags |= DESCFLAGS_DESC::LOADED;
    return TagData::GetDDescription(node, description, flags);
}

Bool MeshCheckTagData::GetDEnabling(const GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) const
{
    if (!node)
        return false;

    const BaseTag* tag = static_cast<const BaseTag*>(node);
    const BaseContainer* bc = tag->GetDataInstance();
    if (!bc)
        return false;

    const Int32 paramId = id[0].id;
    const Bool useGradient = bc->GetBool(MESHCHECK_USE_GRADIENT, true);
    const Bool showBoundary = bc->GetBool(MESHCHECK_SHOW_BOUNDARY, false);
    const Bool isEnabled = bc->GetBool(MESHCHECK_ENABLED, true);

    if (paramId != MESHCHECK_ENABLED && !isEnabled)
        return false;

    if (paramId == MESHCHECK_COLOR_MIN || paramId == MESHCHECK_COLOR_MAX)
        return useGradient;

    if (paramId == MESHCHECK_EDGE_COLOR)
        return !useGradient;

    if (paramId == MESHCHECK_BOUNDARY_COLOR)
        return showBoundary;

    return TagData::GetDEnabling(node, id, t_data, flags, itemdesc);
}

const PolygonObject* MeshCheckTagData::GetPolygonObject(BaseObject* op) const
{
    if (!op)
        return nullptr;

    if (op->GetDeformCache() && op->GetDeformCache()->IsInstanceOf(Opolygon))
        return static_cast<const PolygonObject*>(op->GetDeformCache());

    if (op->IsInstanceOf(Opolygon))
        return static_cast<const PolygonObject*>(op);

    if (op->GetCache() && op->GetCache()->IsInstanceOf(Opolygon))
        return static_cast<const PolygonObject*>(op->GetCache());

    return nullptr;
}

Vector MeshCheckTagData::CalcPolyNormal(const Vector* points, const CPolygon& poly)
{
    const Vector& pa = points[poly.a];
    const Vector& pb = points[poly.b];
    const Vector& pc = points[poly.c];

    if (poly.c == poly.d)
    {
        // Triangle
        Vector e1 = pb - pa;
        Vector e2 = pc - pa;
        Vector n = Cross(e1, e2);
        Float len = n.GetLength();
        return (len > 1e-7) ? (n / len) : Vector(0.0, 1.0, 0.0);
    }
    else
    {
        // Quad — Newell's method for arbitrary (planar or non-planar) quads
        const Vector& pd = points[poly.d];
        Vector n(
            (pa.y - pb.y) * (pa.z + pb.z) + (pb.y - pc.y) * (pb.z + pc.z) + (pc.y - pd.y) * (pc.z + pd.z) + (pd.y - pa.y) * (pd.z + pa.z),
            (pa.z - pb.z) * (pa.x + pb.x) + (pb.z - pc.z) * (pb.x + pc.x) + (pc.z - pd.z) * (pc.x + pd.x) + (pd.z - pa.z) * (pd.x + pa.x),
            (pa.x - pb.x) * (pa.y + pb.y) + (pb.x - pc.x) * (pb.y + pc.y) + (pc.x - pd.x) * (pc.y + pd.y) + (pd.x - pa.x) * (pd.y + pa.y)
        );
        Float len = n.GetLength();
        return (len > 1e-7) ? (n / len) : Vector(0.0, 1.0, 0.0);
    }
}

void MeshCheckTagData::UpdateMeshEdges(BaseTag* tag, BaseObject* op, Bool forceRecalculate) const
{
    if (!tag || !op)
        return;

    const BaseContainer* data = tag->GetDataInstance();
    if (!data)
        return;

    const Bool enabled = data->GetBool(MESHCHECK_ENABLED, true);
    const Float threshold = data->GetFloat(MESHCHECK_ANGLE_THRESHOLD, 89.0);
    const Bool showBoundary = data->GetBool(MESHCHECK_SHOW_BOUNDARY, false);

    if (!enabled)
    {
        if (m_highlightEdges.GetCount() > 0)
            m_highlightEdges.Reset();
        m_lastEnabled = false;
        m_problemEdgeCount = 0;
        m_boundaryEdgeCount = 0;
        return;
    }

    const PolygonObject* polyOp = GetPolygonObject(op);
    if (!polyOp)
    {
        m_highlightEdges.Reset();
        m_problemEdgeCount = 0;
        m_boundaryEdgeCount = 0;
        return;
    }

    const UInt64 dirtyChecksum = polyOp->GetDirty(DIRTYFLAGS::DATA);
    const Int32 pointCount = polyOp->GetPointCount();
    const Int32 polyCount = polyOp->GetPolygonCount();

    if (!forceRecalculate &&
        dirtyChecksum == m_lastDirtyChecksum &&
        pointCount == m_lastPointCount &&
        polyCount == m_lastPolyCount &&
        Abs(threshold - m_lastThreshold) < 0.01 &&
        showBoundary == m_lastShowBoundary &&
        enabled == m_lastEnabled)
    {
        return;
    }

    m_lastDirtyChecksum = dirtyChecksum;
    m_lastPointCount = pointCount;
    m_lastPolyCount = polyCount;
    m_lastThreshold = threshold;
    m_lastShowBoundary = showBoundary;
    m_lastEnabled = enabled;

    m_highlightEdges.Reset();
    m_problemEdgeCount = 0;
    m_boundaryEdgeCount = 0;

    if (pointCount < 2 || polyCount < 1)
        return;

    const Vector* points = polyOp->GetPointR();
    const CPolygon* polys = polyOp->GetPolygonR();
    if (!points || !polys)
        return;

    // 1. Precalculate normal for every polygon
    maxon::BaseArray<Vector> polyNormals;
    polyNormals.Resize(polyCount) iferr_ignore("Resize");
    if (polyNormals.GetCount() != polyCount)
        return;

    for (Int32 i = 0; i < polyCount; ++i)
    {
        polyNormals[i] = CalcPolyNormal(points, polys[i]);
    }

    // 2. Build edge-to-polygons map using 64-bit packed vertex indices
    maxon::HashMap<UInt64, EdgePolys> edgeMap;

    for (Int32 i = 0; i < polyCount; ++i)
    {
        const CPolygon& p = polys[i];
        const Int32 v[4] = { p.a, p.b, p.c, p.d };
        const Int32 numSides = (p.c == p.d) ? 3 : 4;

        for (Int32 e = 0; e < numSides; ++e)
        {
            const Int32 va = v[e];
            const Int32 vb = v[(e + 1) % numSides];
            if (va == vb)
                continue;

            const Int32 v0 = Min(va, vb);
            const Int32 v1 = Max(va, vb);
            const UInt64 key = (UInt64(UInt32(v0)) << 32) | UInt64(UInt32(v1));

            auto* entry = edgeMap.Find(key);
            if (entry)
            {
                if (entry->GetValue().polyCount == 1)
                    entry->GetValue().poly1 = i;
                entry->GetValue().polyCount++;
            }
            else
            {
                EdgePolys newEntry;
                newEntry.poly0 = i;
                newEntry.polyCount = 1;
                edgeMap.Insert(key, newEntry) iferr_ignore("insert edge");
            }
        }
    }

    // 3. Scan all edges and check angle or boundary status
    for (const auto& entry : edgeMap)
    {
        const UInt64 key = entry.GetKey();
        const EdgePolys& ep = entry.GetValue();
        const Int32 v0 = Int32(key >> 32);
        const Int32 v1 = Int32(key & 0xFFFFFFFF);

        if (ep.polyCount == 2)
        {
            const Vector& n0 = polyNormals[ep.poly0];
            const Vector& n1 = polyNormals[ep.poly1];
            Float dot = Dot(n0, n1);
            dot = ClampValue(dot, -1.0, 1.0);
            Float angleDeg = RadToDeg(ACos(dot));

            if (angleDeg >= threshold)
            {
                HighlightEdge he;
                he.v0 = v0;
                he.v1 = v1;
                he.angle = angleDeg;
                he.isBoundary = false;
                he.isNonManifold = false;
                m_highlightEdges.Append(he) iferr_ignore("append problem edge");
                m_problemEdgeCount++;
            }
        }
        else if (ep.polyCount == 1)
        {
            m_boundaryEdgeCount++;
            if (showBoundary)
            {
                HighlightEdge he;
                he.v0 = v0;
                he.v1 = v1;
                he.angle = 0.0;
                he.isBoundary = true;
                he.isNonManifold = false;
                m_highlightEdges.Append(he) iferr_ignore("append boundary edge");
            }
        }
        else if (ep.polyCount > 2)
        {
            // Non-manifold edge
            HighlightEdge he;
            he.v0 = v0;
            he.v1 = v1;
            he.angle = 180.0;
            he.isBoundary = false;
            he.isNonManifold = true;
            m_highlightEdges.Append(he) iferr_ignore("append nonmanifold edge");
            m_problemEdgeCount++;
        }
    }

    // 4. Update status display string in tag properties
    String infoStr = FormatString("Problem edges: @ | Boundary: @"_s, m_problemEdgeCount, m_boundaryEdgeCount);
    tag->SetParameter(ConstDescID(DescLevel(MESHCHECK_INFO_COUNT)), GeData(infoStr), DESCFLAGS_SET::NONE);
}

EXECUTIONRESULT MeshCheckTagData::Execute(BaseTag* tag, BaseDocument* doc, BaseObject* op, BaseThread* bt, Int32 priority, EXECUTIONFLAGS flags)
{
    if (!tag || !op)
        return EXECUTIONRESULT::OK;

    UpdateMeshEdges(tag, op, false);
    return EXECUTIONRESULT::OK;
}

Bool MeshCheckTagData::Draw(BaseTag* tag, BaseObject* op, BaseDraw* bd, BaseDrawHelp* bh)
{
    if (!tag || !op || !bd || !bh)
        return true;

    if (bd->GetDrawPass() != DRAWPASS::OBJECT)
        return true;

    if (op->GetEditorMode() == MODE_OFF)
        return true;

    const BaseContainer* data = tag->GetDataInstance();
    if (!data)
        return true;

    if (!data->GetBool(MESHCHECK_ENABLED, true))
        return true;

    UpdateMeshEdges(tag, op, false);

    if (m_highlightEdges.GetCount() == 0)
        return true;

    const PolygonObject* polyOp = GetPolygonObject(op);
    if (!polyOp)
        return true;

    const Vector* points = polyOp->GetPointR();
    if (!points)
        return true;

    const Float threshold = data->GetFloat(MESHCHECK_ANGLE_THRESHOLD, 89.0);
    Float lineWidth = data->GetFloat(MESHCHECK_EDGE_WIDTH, 2.5);
    if (lineWidth < 1.0) lineWidth = 1.0;
    if (lineWidth > 10.0) lineWidth = 10.0;

    const Bool useGradient = data->GetBool(MESHCHECK_USE_GRADIENT, true);
    const Vector edgeColor = data->GetVector(MESHCHECK_EDGE_COLOR, Vector(1.0, 0.2, 0.0));
    const Vector colMin = data->GetVector(MESHCHECK_COLOR_MIN, Vector(1.0, 0.85, 0.0));
    const Vector colMax = data->GetVector(MESHCHECK_COLOR_MAX, Vector(1.0, 0.05, 0.05));
    const Bool depthTest = data->GetBool(MESHCHECK_DEPTH_TEST, true);
    const Vector boundaryColor = data->GetVector(MESHCHECK_BOUNDARY_COLOR, Vector(0.0, 0.75, 1.0));

    bd->SetMatrix_Matrix(nullptr, Matrix());

    GeData oldLineWidth = bd->GetDrawParam(DRAW_PARAMETER_LINEWIDTH);
    GeData oldUseZ = bd->GetDrawParam(DRAW_PARAMETER_USE_Z);
    GeData oldSetZ = bd->GetDrawParam(DRAW_PARAMETER_SETZ);

    bd->SetDrawParam(DRAW_PARAMETER_LINEWIDTH, GeData(lineWidth));
    if (depthTest)
    {
        bd->SetDrawParam(DRAW_PARAMETER_USE_Z, GeData(true));
        bd->SetDrawParam(DRAW_PARAMETER_SETZ, GeData(DRAW_Z_LOWEREQUAL));
        bd->LineZOffset(2);
    }
    else
    {
        bd->SetDrawParam(DRAW_PARAMETER_USE_Z, GeData(false));
    }

    const Matrix& opMg = op->GetMg();

    auto drawThickLine = [&](const Vector& p1, const Vector& p2, const Vector& col, Float width)
    {
        bd->SetPen(col, SET_PEN_USE_PROFILE_COLOR);
        bd->DrawLine(p1, p2, 0);

        if (width >= 1.35)
        {
            Vector s1 = bd->WS(p1);
            Vector s2 = bd->WS(p2);
            Vector dir = Vector(s2.x - s1.x, s2.y - s1.y, 0.0);
            Float len = Sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 0.001)
            {
                Vector perp(-dir.y / len, dir.x / len, 0.0);
                Int32 extraSteps = (Int32)Floor((width - 0.7) * 0.8) + 1;
                if (extraSteps < 1) extraSteps = 1;
                if (extraSteps > 5) extraSteps = 5;

                for (Int32 step = 1; step <= extraSteps; ++step)
                {
                    Float offset = Float(step);
                    Vector p1_a = bd->SW(Vector(s1.x + perp.x * offset, s1.y + perp.y * offset, s1.z));
                    Vector p2_a = bd->SW(Vector(s2.x + perp.x * offset, s2.y + perp.y * offset, s2.z));
                    Vector p1_b = bd->SW(Vector(s1.x - perp.x * offset, s1.y - perp.y * offset, s1.z));
                    Vector p2_b = bd->SW(Vector(s2.x - perp.x * offset, s2.y - perp.y * offset, s2.z));
                    bd->DrawLine(p1_a, p2_a, 0);
                    bd->DrawLine(p1_b, p2_b, 0);
                }
            }
        }
    };

    for (Int32 i = 0; i < m_highlightEdges.GetCount(); ++i)
    {
        const HighlightEdge& he = m_highlightEdges[i];
        Vector p0 = opMg * points[he.v0];
        Vector p1 = opMg * points[he.v1];

        if (he.isBoundary)
        {
            drawThickLine(p0, p1, boundaryColor, lineWidth);
        }
        else if (he.isNonManifold)
        {
            drawThickLine(p0, p1, Vector(1.0, 0.0, 1.0), lineWidth);
        }
        else
        {
            Vector col = edgeColor;
            if (useGradient)
            {
                Float t = 0.0;
                if (threshold < 180.0)
                    t = (he.angle - threshold) / (180.0 - threshold);
                t = ClampValue(t, 0.0, 1.0);
                col = (1.0 - t) * colMin + t * colMax;
            }
            drawThickLine(p0, p1, col, lineWidth);
        }
    }

    if (depthTest)
        bd->LineZOffset(0);

    bd->SetDrawParam(DRAW_PARAMETER_LINEWIDTH, oldLineWidth);
    bd->SetDrawParam(DRAW_PARAMETER_USE_Z, oldUseZ);
    bd->SetDrawParam(DRAW_PARAMETER_SETZ, oldSetZ);

    return true;
}

Bool MeshCheckTagData::Message(GeListNode* node, Int32 type, void* data)
{
    if (type == MSG_DESCRIPTION_POSTSETPARAMETER)
    {
        EventAdd();
    }
    return TagData::Message(node, type, data);
}

// -------------------------------------------------------------------------
// MeshCheckCommand Implementation
// -------------------------------------------------------------------------

Bool MeshCheckCommand::Execute(BaseDocument* doc, GeDialog* parentManager)
{
    if (!doc)
        return false;

    BaseObject* op = doc->GetActiveObject();
    if (!op)
    {
        MessageDialog("Please select an object first."_s);
        return true;
    }

    // Check if tag already exists on active object
    BaseTag* existingTag = nullptr;
    for (BaseTag* tag = op->GetFirstTag(); tag; tag = tag->GetNext())
    {
        if (tag->GetType() == PLUGIN_ID_MESHCHECK_TAG)
        {
            existingTag = tag;
            break;
        }
    }

    doc->StartUndo();
    if (existingTag)
    {
        BaseContainer* bc = existingTag->GetDataInstance();
        if (bc)
        {
            doc->AddUndo(UNDOTYPE::CHANGE, existingTag);
            Bool isEn = bc->GetBool(MESHCHECK_ENABLED, true);
            bc->SetBool(MESHCHECK_ENABLED, !isEn);
        }
        doc->SetActiveTag(existingTag);
    }
    else
    {
        BaseTag* newTag = BaseTag::Alloc(PLUGIN_ID_MESHCHECK_TAG);
        if (newTag)
        {
            op->InsertTag(newTag);
            doc->AddUndo(UNDOTYPE::NEWOBJ, newTag);
            doc->SetActiveTag(newTag);
        }
    }
    doc->EndUndo();
    EventAdd();

    return true;
}

// -------------------------------------------------------------------------
// Plugin Registration
// -------------------------------------------------------------------------

Bool RegisterMeshCheckTag()
{
    return RegisterTagPlugin(
        PLUGIN_ID_MESHCHECK_TAG,
        GeLoadString(IDS_MESHCHECK_TAG),
        TAG_VISIBLE | TAG_EXPRESSION | TAG_IMPLEMENTS_DRAW_FUNCTION,
        MeshCheckTagData::Alloc,
        "Tmeshcheck"_s,
        AutoBitmap("meshcheck.png"_s),
        0
    );
}

Bool RegisterMeshCheckCommand()
{
    return RegisterCommandPlugin(
        PLUGIN_ID_MESHCHECK_COMMAND,
        GeLoadString(IDS_MESHCHECK_COMMAND),
        0,
        AutoBitmap("meshcheck.png"_s),
        "Toggle MeshCheck Tag on Active Object"_s,
        NewObjClear(MeshCheckCommand)
    );
}

} // namespace cinema
