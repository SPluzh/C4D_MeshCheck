#include "meshcheck_tag.h"
#include "c4d_symbols.h"
#include "description/tmeshcheck.h"
#include "description/tphong.h"
#include "c4d_painter.h"

namespace cinema
{

struct EdgePolys
{
    Int32 poly0 = NOTOK;
    Int32 poly1 = NOTOK;
    Int32 edgeIndex0 = NOTOK;
    Int32 edgeIndex1 = NOTOK;
    Int32 polyCount = 0;
    Vector uv0_v0 = Vector(0.0);
    Vector uv0_v1 = Vector(0.0);
    Vector uv1_v0 = Vector(0.0);
    Vector uv1_v1 = Vector(0.0);
    Vector nrm0_v0 = Vector(0.0);
    Vector nrm0_v1 = Vector(0.0);
    Vector nrm1_v0 = Vector(0.0);
    Vector nrm1_v1 = Vector(0.0);
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
        data->SetFloat(MESHCHECK_EDGE_WIDTH, 2.5);
        data->SetBool(MESHCHECK_DEPTH_TEST, true);

        // Dihedral angle
        data->SetBool(MESHCHECK_SHOW_ANGLE, true);
        data->SetFloat(MESHCHECK_ANGLE_THRESHOLD, DegToRad(89.0));
        data->SetBool(MESHCHECK_ANGLE_IGNORE_HARD, false);
        data->SetBool(MESHCHECK_USE_GRADIENT, true);
        data->SetVector(MESHCHECK_EDGE_COLOR, Vector(1.0, 0.2, 0.0));
        data->SetVector(MESHCHECK_COLOR_MIN, Vector(1.0, 0.85, 0.0));
        data->SetVector(MESHCHECK_COLOR_MAX, Vector(1.0, 0.05, 0.05));

        // Hard edges
        data->SetBool(MESHCHECK_SHOW_HARD_EDGES, true);
        data->SetVector(MESHCHECK_HARD_EDGE_COLOR, Vector(0.15, 0.55, 1.0));

        // UV seams
        data->SetBool(MESHCHECK_SHOW_UV_SEAMS, true);
        data->SetVector(MESHCHECK_UV_SEAM_COLOR, Vector(0.2, 0.85, 0.3));

        // Combined Hard & UV seam
        data->SetBool(MESHCHECK_SHOW_HARD_SEAM_DIFF, true);
        data->SetVector(MESHCHECK_HARD_SEAM_COLOR, Vector(0.7, 0.25, 0.95));

        // Boundary
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
    const Bool isEnabled = bc->GetBool(MESHCHECK_ENABLED, true);

    if (paramId != MESHCHECK_ENABLED && !isEnabled)
        return false;

    const Bool showAngle = bc->GetBool(MESHCHECK_SHOW_ANGLE, true);
    const Bool useGradient = bc->GetBool(MESHCHECK_USE_GRADIENT, true);
    const Bool showHardEdges = bc->GetBool(MESHCHECK_SHOW_HARD_EDGES, true);
    const Bool showUVSeams = bc->GetBool(MESHCHECK_SHOW_UV_SEAMS, true);
    const Bool showHardSeamDiff = bc->GetBool(MESHCHECK_SHOW_HARD_SEAM_DIFF, true);
    const Bool showBoundary = bc->GetBool(MESHCHECK_SHOW_BOUNDARY, false);

    // Dihedral Angle parameters
    if (paramId == MESHCHECK_ANGLE_THRESHOLD || paramId == MESHCHECK_USE_GRADIENT || paramId == MESHCHECK_ANGLE_IGNORE_HARD)
        return showAngle;

    if (paramId == MESHCHECK_COLOR_MIN || paramId == MESHCHECK_COLOR_MAX)
        return showAngle && useGradient;

    if (paramId == MESHCHECK_EDGE_COLOR)
        return showAngle && !useGradient;

    // Hard Edges parameters
    if (paramId == MESHCHECK_HARD_EDGE_COLOR)
        return showHardEdges;

    // UV Seams parameters
    if (paramId == MESHCHECK_UV_SEAM_COLOR)
        return showUVSeams;

    // Combined Hard + Seams parameters
    if (paramId == MESHCHECK_SHOW_HARD_SEAM_DIFF)
        return showHardEdges && showUVSeams;

    if (paramId == MESHCHECK_HARD_SEAM_COLOR)
        return showHardEdges && showUVSeams && showHardSeamDiff;

    // Boundary parameters
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
    const Bool showAngle = data->GetBool(MESHCHECK_SHOW_ANGLE, true);
    Float thresholdRad = data->GetFloat(MESHCHECK_ANGLE_THRESHOLD, DegToRad(89.0));
    if (thresholdRad > maxon::PI)
        thresholdRad = DegToRad(thresholdRad);
    const Float thresholdDeg = RadToDeg(thresholdRad);
    const Bool angleIgnoreHard = data->GetBool(MESHCHECK_ANGLE_IGNORE_HARD, false);
    const Bool showHardEdges = data->GetBool(MESHCHECK_SHOW_HARD_EDGES, true);
    const Bool showUVSeams = data->GetBool(MESHCHECK_SHOW_UV_SEAMS, true);
    const Bool showBoundary = data->GetBool(MESHCHECK_SHOW_BOUNDARY, false);

    if (!enabled)
    {
        if (m_highlightEdges.GetCount() > 0)
            m_highlightEdges.Reset();
        m_lastEnabled = false;
        m_problemEdgeCount = 0;
        m_boundaryEdgeCount = 0;
        m_hardEdgeCount = 0;
        m_uvSeamCount = 0;
        return;
    }

    const PolygonObject* polyOp = GetPolygonObject(op);
    if (!polyOp)
    {
        m_highlightEdges.Reset();
        m_problemEdgeCount = 0;
        m_boundaryEdgeCount = 0;
        m_hardEdgeCount = 0;
        m_uvSeamCount = 0;
        return;
    }

    const Int32 pointCount = polyOp->GetPointCount();
    const Int32 polyCount = polyOp->GetPolygonCount();

    const UVWTag* uvwTag = static_cast<const UVWTag*>(polyOp->GetTag(Tuvw));
    const BaseTag* phongTag = polyOp->GetTag(Tphong);
    const NormalTag* normalTag = static_cast<const NormalTag*>(polyOp->GetTag(Tnormal));

    UInt64 dirtyChecksum = UInt64(polyOp->GetDirty(DIRTYFLAGS::DATA | DIRTYFLAGS::SELECT));
    if (uvwTag)
        dirtyChecksum ^= (UInt64(uvwTag->GetDirty(DIRTYFLAGS::DATA)) << 11);
    if (phongTag)
        dirtyChecksum ^= (UInt64(phongTag->GetDirty(DIRTYFLAGS::DATA)) << 23);
    if (normalTag)
        dirtyChecksum ^= (UInt64(normalTag->GetDirty(DIRTYFLAGS::DATA)) << 35);
    const EdgeBaseSelect* phongBreaksPreview = polyOp->GetPhongBreak();
    if (phongBreaksPreview)
        dirtyChecksum ^= (UInt64(phongBreaksPreview->GetCount()) << 43) ^ (UInt64(phongBreaksPreview->GetSegments()) << 51);

    if (!forceRecalculate &&
        dirtyChecksum == m_lastDirtyChecksum &&
        pointCount == m_lastPointCount &&
        polyCount == m_lastPolyCount &&
        Abs(thresholdDeg - m_lastThreshold) < 0.01 &&
        showAngle == m_lastShowAngle &&
        angleIgnoreHard == m_lastAngleIgnoreHard &&
        showHardEdges == m_lastShowHardEdges &&
        showUVSeams == m_lastShowUVSeams &&
        showBoundary == m_lastShowBoundary &&
        enabled == m_lastEnabled)
    {
        return;
    }

    m_lastDirtyChecksum = dirtyChecksum;
    m_lastPointCount = pointCount;
    m_lastPolyCount = polyCount;
    m_lastThreshold = thresholdDeg;
    m_lastShowAngle = showAngle;
    m_lastAngleIgnoreHard = angleIgnoreHard;
    m_lastShowHardEdges = showHardEdges;
    m_lastShowUVSeams = showUVSeams;
    m_lastShowBoundary = showBoundary;
    m_lastEnabled = enabled;

    m_highlightEdges.Reset();
    m_problemEdgeCount = 0;
    m_boundaryEdgeCount = 0;
    m_hardEdgeCount = 0;
    m_uvSeamCount = 0;

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

    // 2. Query UV, Phong, Normal data handles
    ConstUVWHandle uvData = uvwTag ? uvwTag->GetDataAddressR() : nullptr;
    ConstNormalHandle normalData = normalTag ? normalTag->GetDataAddressR() : nullptr;

    const EdgeBaseSelect* phongBreaks = polyOp->GetPhongBreak();
    const Bool hasPhongBreaks = (phongBreaks != nullptr && phongBreaks->GetCount() > 0);

    const EdgeBaseSelect* uvSeamsSel = GetUVSeams2(polyOp, false);
    const Bool hasUVSeamsSel = (uvSeamsSel != nullptr && uvSeamsSel->GetCount() > 0);

    // 3. Build edge-to-polygons map using 64-bit packed vertex indices
    maxon::HashMap<UInt64, EdgePolys> edgeMap;

    for (Int32 i = 0; i < polyCount; ++i)
    {
        const CPolygon& p = polys[i];
        const Int32 v[4] = { p.a, p.b, p.c, p.d };
        const Int32 numSides = (p.c == p.d) ? 3 : 4;

        UVWStruct uvw;
        if (uvData != nullptr)
        {
            UVWTag::Get(uvData, i, uvw);
        }

        NormalStruct nrm;
        if (normalData != nullptr)
        {
            NormalTag::Get(normalData, i, nrm);
        }

        for (Int32 e = 0; e < numSides; ++e)
        {
            const Int32 va = v[e];
            const Int32 vb = v[(e + 1) % numSides];
            if (va == vb)
                continue;

            const Int32 v0 = Min(va, vb);
            const Int32 v1 = Max(va, vb);
            const UInt64 key = (UInt64(UInt32(v0)) << 32) | UInt64(UInt32(v1));

            // Cinema 4D edge index: for triangles, edge from c to a is index 3
            const Int32 c4dEdgeIndex = (numSides == 3 && e == 2) ? 3 : e;

            Vector uv_v0(0.0), uv_v1(0.0);
            if (uvData != nullptr)
            {
                uv_v0 = (va == v0) ? uvw[e] : uvw[(e + 1) % numSides];
                uv_v1 = (va == v1) ? uvw[e] : uvw[(e + 1) % numSides];
            }

            Vector nrm_v0(0.0), nrm_v1(0.0);
            if (normalData != nullptr)
            {
                nrm_v0 = (va == v0) ? nrm[e] : nrm[(e + 1) % numSides];
                nrm_v1 = (va == v1) ? nrm[e] : nrm[(e + 1) % numSides];
            }

            auto* entry = edgeMap.Find(key);
            if (entry)
            {
                if (entry->GetValue().polyCount == 1)
                {
                    entry->GetValue().poly1 = i;
                    entry->GetValue().edgeIndex1 = c4dEdgeIndex;
                    entry->GetValue().uv1_v0 = uv_v0;
                    entry->GetValue().uv1_v1 = uv_v1;
                    entry->GetValue().nrm1_v0 = nrm_v0;
                    entry->GetValue().nrm1_v1 = nrm_v1;
                }
                entry->GetValue().polyCount++;
            }
            else
            {
                EdgePolys newEntry;
                newEntry.poly0 = i;
                newEntry.edgeIndex0 = c4dEdgeIndex;
                newEntry.polyCount = 1;
                newEntry.uv0_v0 = uv_v0;
                newEntry.uv0_v1 = uv_v1;
                newEntry.nrm0_v0 = nrm_v0;
                newEntry.nrm0_v1 = nrm_v1;
                edgeMap.Insert(key, newEntry) iferr_ignore("insert edge");
            }
        }
    }

    // 4. Scan all edges and check angle, hard edges, uv seams, boundary, or non-manifold
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

            Bool isHard = false;
            if (showHardEdges || angleIgnoreHard)
            {
                if (hasPhongBreaks && (phongBreaks->IsSelected(4 * ep.poly0 + ep.edgeIndex0) || phongBreaks->IsSelected(4 * ep.poly1 + ep.edgeIndex1)))
                {
                    isHard = true;
                }
                else if (normalData != nullptr)
                {
                    Float d0 = Dot(ep.nrm0_v0, ep.nrm1_v0);
                    Float d1 = Dot(ep.nrm0_v1, ep.nrm1_v1);
                    if (d0 < 0.999 || d1 < 0.999)
                        isHard = true;
                }

                if (isHard && showHardEdges)
                    m_hardEdgeCount++;
            }

            Bool isAngle = false;
            if (showAngle && angleDeg >= thresholdDeg)
            {
                if (!(angleIgnoreHard && isHard))
                {
                    isAngle = true;
                    m_problemEdgeCount++;
                }
            }

            Bool isSeam = false;
            if (showUVSeams)
            {
                if (hasUVSeamsSel && (uvSeamsSel->IsSelected(4 * ep.poly0 + ep.edgeIndex0) || uvSeamsSel->IsSelected(4 * ep.poly1 + ep.edgeIndex1)))
                {
                    isSeam = true;
                }
                else if (uvData != nullptr)
                {
                    Float du0 = Abs(ep.uv0_v0.x - ep.uv1_v0.x);
                    Float dv0 = Abs(ep.uv0_v0.y - ep.uv1_v0.y);
                    Float du1 = Abs(ep.uv0_v1.x - ep.uv1_v1.x);
                    Float dv1 = Abs(ep.uv0_v1.y - ep.uv1_v1.y);
                    if (du0 > 1e-4 || dv0 > 1e-4 || du1 > 1e-4 || dv1 > 1e-4)
                        isSeam = true;
                }

                if (isSeam)
                    m_uvSeamCount++;
            }

            if (isAngle || (isHard && showHardEdges) || isSeam)
            {
                HighlightEdge he;
                he.v0 = v0;
                he.v1 = v1;
                he.angle = angleDeg;
                he.isBoundary = false;
                he.isNonManifold = false;
                he.isAngleProblem = isAngle;
                he.isHardEdge = isHard;
                he.isUVSeam = isSeam;
                m_highlightEdges.Append(he) iferr_ignore("append highlight edge");
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
                he.isAngleProblem = false;
                he.isHardEdge = false;
                he.isUVSeam = false;
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
            he.isAngleProblem = false;
            he.isHardEdge = false;
            he.isUVSeam = false;
            m_highlightEdges.Append(he) iferr_ignore("append nonmanifold edge");
            m_problemEdgeCount++;
        }
    }

    // 5. Update status display string in tag properties
    String infoStr = FormatString("Angle: @ | Hard: @ | UV: @ | Boundary: @"_s,
        m_problemEdgeCount, m_hardEdgeCount, m_uvSeamCount, m_boundaryEdgeCount);
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

    const Bool showAngle = data->GetBool(MESHCHECK_SHOW_ANGLE, true);
    Float thresholdRad = data->GetFloat(MESHCHECK_ANGLE_THRESHOLD, DegToRad(89.0));
    if (thresholdRad > maxon::PI)
        thresholdRad = DegToRad(thresholdRad);
    const Float thresholdDeg = RadToDeg(thresholdRad);
    Float lineWidth = data->GetFloat(MESHCHECK_EDGE_WIDTH, 2.5);
    if (lineWidth < 1.0) lineWidth = 1.0;
    if (lineWidth > 10.0) lineWidth = 10.0;

    const Bool useGradient = data->GetBool(MESHCHECK_USE_GRADIENT, true);
    const Vector edgeColor = data->GetVector(MESHCHECK_EDGE_COLOR, Vector(1.0, 0.2, 0.0));
    const Vector colMin = data->GetVector(MESHCHECK_COLOR_MIN, Vector(1.0, 0.85, 0.0));
    const Vector colMax = data->GetVector(MESHCHECK_COLOR_MAX, Vector(1.0, 0.05, 0.05));
    const Bool depthTest = data->GetBool(MESHCHECK_DEPTH_TEST, true);

    const Bool showHardEdges = data->GetBool(MESHCHECK_SHOW_HARD_EDGES, true);
    const Vector hardEdgeColor = data->GetVector(MESHCHECK_HARD_EDGE_COLOR, Vector(0.15, 0.55, 1.0));

    const Bool showUVSeams = data->GetBool(MESHCHECK_SHOW_UV_SEAMS, true);
    const Vector uvSeamColor = data->GetVector(MESHCHECK_UV_SEAM_COLOR, Vector(0.2, 0.85, 0.3));

    const Bool showHardSeamDiff = data->GetBool(MESHCHECK_SHOW_HARD_SEAM_DIFF, true);
    const Vector hardSeamColor = data->GetVector(MESHCHECK_HARD_SEAM_COLOR, Vector(0.7, 0.25, 0.95));

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
            Vector col;
            if (he.isHardEdge && he.isUVSeam && showHardEdges && showUVSeams && showHardSeamDiff)
            {
                col = hardSeamColor;
            }
            else if (he.isHardEdge && showHardEdges)
            {
                col = hardEdgeColor;
            }
            else if (he.isUVSeam && showUVSeams)
            {
                col = uvSeamColor;
            }
            else if (he.isAngleProblem && showAngle)
            {
                col = edgeColor;
                if (useGradient)
                {
                    Float t = 0.0;
                    if (thresholdDeg < 180.0)
                        t = (he.angle - thresholdDeg) / (180.0 - thresholdDeg);
                    t = ClampValue(t, 0.0, 1.0);
                    col = (1.0 - t) * colMin + t * colMax;
                }
            }
            else
            {
                continue;
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
    String name = GeLoadString(IDS_MESHCHECK_TAG);
    if (name.IsEmpty() || name == "StrNotFound"_s)
        name = "MeshCheck"_s;

    return RegisterTagPlugin(
        PLUGIN_ID_MESHCHECK_TAG,
        name,
        TAG_VISIBLE | TAG_EXPRESSION | TAG_IMPLEMENTS_DRAW_FUNCTION,
        MeshCheckTagData::Alloc,
        "Tmeshcheck"_s,
        AutoBitmap("meshcheck.png"_s),
        0
    );
}

Bool RegisterMeshCheckCommand()
{
    String name = GeLoadString(IDS_MESHCHECK_COMMAND);
    if (name.IsEmpty() || name == "StrNotFound"_s)
        name = "Toggle MeshCheck Tag"_s;

    return RegisterCommandPlugin(
        PLUGIN_ID_MESHCHECK_COMMAND,
        name,
        0,
        AutoBitmap("meshcheck.png"_s),
        "Toggle MeshCheck Tag on Active Object"_s,
        NewObjClear(MeshCheckCommand)
    );
}

} // namespace cinema
