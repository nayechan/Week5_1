#include "pch.h"
#include "Global/Types.h"
#include "Utility/Public/MeshSimplifier.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>

// Helper function to create a quadric matrix from a plane
FMatrix QuadricFromPlane(const FVector& point, const FVector& normal)
{
	// Plane equation: ax + by + cz + d = 0
	float a = normal.X;
	float b = normal.Y;
	float c = normal.Z;
	float d = -normal.Dot(point);

	// Quadric matrix Q = [a b c d]^T * [a b c d]
	return FMatrix(
		a * a, a * b, a * c, a * d,
		a * b, b * b, b * c, b * d,
		a * c, b * c, c * c, c * d,
		a * d, b * d, c * d, d * d
	);
}

// Calculate quadric error for a point
float CalculateQuadricError(const FVector& point, const FMatrix& quadric)
{
	// Error = [x y z 1] * Q * [x y z 1]^T
	float x = point.X, y = point.Y, z = point.Z;

	// First multiply: [x y z 1] * Q
	float qx = quadric.Data[0][0] * x + quadric.Data[1][0] * y + quadric.Data[2][0] * z + quadric.Data[3][0];
	float qy = quadric.Data[0][1] * x + quadric.Data[1][1] * y + quadric.Data[2][1] * z + quadric.Data[3][1];
	float qz = quadric.Data[0][2] * x + quadric.Data[1][2] * y + quadric.Data[2][2] * z + quadric.Data[3][2];
	float qw = quadric.Data[0][3] * x + quadric.Data[1][3] * y + quadric.Data[2][3] * z + quadric.Data[3][3];

	// Second multiply: result * [x y z 1]^T
	return qx * x + qy * y + qz * z + qw;
}

// Get shared face count between two vertices
int GetSharedFaceCount(int v1, int v2, const TArray<UMeshSimplifier::FSimplifyFace>& faces)
{
	int sharedFaceCount = 0;
	for (const auto& face : faces)
	{
		if (!face.bIsActive) continue;

		bool hasV1 = (face.V[0] == v1 || face.V[1] == v1 || face.V[2] == v1);
		bool hasV2 = (face.V[0] == v2 || face.V[1] == v2 || face.V[2] == v2);

		if (hasV1 && hasV2)
			++sharedFaceCount;
	}
	return sharedFaceCount;
}

// Check if an edge is a boundary edge (shared by only one face)
bool IsBoundaryEdge(int v1, int v2, const TArray<UMeshSimplifier::FSimplifyFace>& faces)
{
	return GetSharedFaceCount(v1, v2, faces) == 1;
}

// Get common neighbors between two vertices
TArray<int> GetCommonNeighbors(int v1, int v2, const std::vector<std::unordered_set<int>>& adjacency)
{
	TArray<int> commonNeighbors;
	for (int neighbor : adjacency[v1])
	{
		if (adjacency[v2].count(neighbor) > 0 && neighbor != v1 && neighbor != v2)
			commonNeighbors.push_back(neighbor);
	}
	return commonNeighbors;
}

// ULTRA-CONSERVATIVE: Check if any vertex is on boundary
bool IsVertexOnBoundary(int vertexIndex, const TArray<UMeshSimplifier::FSimplifyFace>& faces,
	const std::vector<std::unordered_set<int>>& adjacency)
{
	// Count how many faces each edge from this vertex appears in
	for (int neighbor : adjacency[vertexIndex])
	{
		int edgeFaceCount = GetSharedFaceCount(vertexIndex, neighbor, faces);
		if (edgeFaceCount == 1) // Boundary edge found
			return true;
	}
	return false;
}

// Check if edge collapse is valid (maintains manifold and watertight properties)
bool IsValidEdgeCollapse(int v1, int v2, const TArray<UMeshSimplifier::FSimplifyFace>& faces,
	const std::vector<std::unordered_set<int>>& adjacency, const TArray<UMeshSimplifier::FSimplifyVertex>& vertices,
	const FVector& newPosition)
{
	// ULTRA-CONSERVATIVE POLICY: Don't collapse any edge involving boundary vertices
	if (IsVertexOnBoundary(v1, faces, adjacency) || IsVertexOnBoundary(v2, faces, adjacency))
		return false;

	int sharedFaceCount = GetSharedFaceCount(v1, v2, faces);

	// Only allow interior edges (shared by exactly 2 faces)
	if (sharedFaceCount != 2)
		return false;

	// Must have exactly 2 common neighbors to maintain manifold property
	TArray<int> commonNeighbors = GetCommonNeighbors(v1, v2, adjacency);
	if (commonNeighbors.size() != 2)
		return false;

	// Note: Face inversion check removed for simplicity and safety

	return true;
}

FStaticMesh* UMeshSimplifier::Simplify(const FStaticMesh* InOriginalMesh, float InTargetRatio)
{
	if (!InOriginalMesh || InTargetRatio <= 0.0f || InTargetRatio >= 1.0f)
	{
		return nullptr;
	}

	// 1. Initialize internal data structures
	TArray<FSimplifyVertex> vertices;
	TArray<FSimplifyFace> faces;

	// Copy vertices
	vertices.reserve(InOriginalMesh->Vertices.size());
	for (const auto& v : InOriginalMesh->Vertices)
	{
		FSimplifyVertex sv;
		sv.Position = v.Position;
		sv.Normal = v.Normal;
		sv.Color = v.Color;
		sv.TexCoord = v.TexCoord;
		sv.Quadric = FMatrix(); // Initialize to zero matrix
		sv.bIsActive = true;
		vertices.push_back(sv);
	}

	// Copy faces
	faces.reserve(InOriginalMesh->Indices.size() / 3);
	for (size_t i = 0; i < InOriginalMesh->Indices.size(); i += 3)
	{
		FSimplifyFace sf;
		sf.V[0] = InOriginalMesh->Indices[i];
		sf.V[1] = InOriginalMesh->Indices[i + 1];
		sf.V[2] = InOriginalMesh->Indices[i + 2];
		sf.bIsActive = true;
		faces.push_back(sf);
	}

	// Build adjacency information
	std::vector<std::unordered_set<int>> adjacency(vertices.size());
	for (const auto& face : faces)
	{
		if (!face.bIsActive) continue;

		int v0 = face.V[0], v1 = face.V[1], v2 = face.V[2];

		adjacency[v0].insert(v1); adjacency[v1].insert(v0);
		adjacency[v1].insert(v2); adjacency[v2].insert(v1);
		adjacency[v2].insert(v0); adjacency[v0].insert(v2);
	}

	// 2. Calculate initial quadrics for each vertex
	for (const auto& face : faces)
	{
		if (!face.bIsActive) continue;

		const FVector& p0 = vertices[face.V[0]].Position;
		const FVector& p1 = vertices[face.V[1]].Position;
		const FVector& p2 = vertices[face.V[2]].Position;

		// Calculate face normal
		FVector edge1 = p1 - p0;
		FVector edge2 = p2 - p0;
		FVector normal = edge1.Cross(edge2);

		if (normal.Length() < 1e-6f) continue; // Skip degenerate faces
		normal.Normalize();

		// Create quadric for this face
		FMatrix faceQuadric = QuadricFromPlane(p0, normal);

		// Add to each vertex's quadric
		vertices[face.V[0]].Quadric += faceQuadric;
		vertices[face.V[1]].Quadric += faceQuadric;
		vertices[face.V[2]].Quadric += faceQuadric;
	}

	// 3. Create priority queue of edge collapse candidates
	std::priority_queue<FCollapseCandidate, std::vector<FCollapseCandidate>, std::greater<FCollapseCandidate>> edgeQueue;

	// Helper to add edge to queue
	auto addEdgeToQueue = [&](int v1, int v2) {
		if (v1 == v2) return;
		if (!vertices[v1].bIsActive || !vertices[v2].bIsActive) return;

		// Combined quadric
		FMatrix combinedQuadric = vertices[v1].Quadric + vertices[v2].Quadric;

		// SAFE POSITION: Use midpoint (simple and predictable)
		FVector midpoint = (vertices[v1].Position + vertices[v2].Position) * 0.5f;
		float error = CalculateQuadricError(midpoint, combinedQuadric);

		FCollapseCandidate candidate;
		candidate.V1 = v1;
		candidate.V2 = v2;
		candidate.Cost = error;
		candidate.OptimalPosition = midpoint;

		edgeQueue.push(candidate);
		};

	// Add all edges to queue
	for (int i = 0; i < vertices.size(); ++i)
	{
		if (!vertices[i].bIsActive) continue;
		for (int neighbor : adjacency[i])
		{
			if (i < neighbor) // Avoid duplicate edges
			{
				addEdgeToQueue(i, neighbor);
			}
		}
	}

	// 4. Perform edge collapses
	int targetFaceCount = static_cast<int>(faces.size() * InTargetRatio);

	// Helper to count active faces accurately
	auto countActiveFaces = [&]() -> int {
		int count = 0;
		for (const auto& face : faces)
		{
			if (face.bIsActive) count++;
		}
		return count;
	};

	while (countActiveFaces() > targetFaceCount && !edgeQueue.empty())
	{
		FCollapseCandidate candidate = edgeQueue.top();
		edgeQueue.pop();

		int v1 = candidate.V1;
		int v2 = candidate.V2;

		// Check if edge is still valid
		if (!vertices[v1].bIsActive || !vertices[v2].bIsActive) continue;
		if (adjacency[v1].find(v2) == adjacency[v1].end()) continue;

		// Check if edge collapse is valid (maintains manifold and watertight properties)
		if (!IsValidEdgeCollapse(v1, v2, faces, adjacency, vertices, candidate.OptimalPosition))
			continue;

		// Perform collapse: merge v2 into v1
		vertices[v1].Position = candidate.OptimalPosition;
		vertices[v1].Quadric = vertices[v1].Quadric + vertices[v2].Quadric;

		// CRITICAL: Interpolate vertex attributes (UV, Normal, Color)
		// Safe normal interpolation for LH Z-up coordinate system
		FVector newNormal = vertices[v1].Normal + vertices[v2].Normal;
		if (newNormal.Length() > 1e-6f) // Avoid zero-length normals
		{
			newNormal.Normalize();
			vertices[v1].Normal = newNormal;
		}
		// If normals cancel out, keep original v1 normal

		vertices[v1].Color = (vertices[v1].Color + vertices[v2].Color) * 0.5f;
		vertices[v1].TexCoord = (vertices[v1].TexCoord + vertices[v2].TexCoord) * 0.5f;

		vertices[v2].bIsActive = false;

		// Update faces: replace v2 with v1
		for (auto& face : faces)
		{
			if (!face.bIsActive) continue;

			bool modified = false;
			for (int j = 0; j < 3; ++j)
			{
				if (face.V[j] == v2)
				{
					face.V[j] = v1;
					modified = true;
				}
			}

			// Check for degenerate faces after modification
			if (modified)
			{
				if (face.V[0] == face.V[1] || face.V[1] == face.V[2] || face.V[2] == face.V[0])
				{
					face.bIsActive = false;
					// Note: activeFaceCount is now calculated dynamically in the main loop
				}
			}
		}

		// SIMPLE ADJACENCY UPDATE: Original method (safer)
		// Update adjacency: move v2's neighbors to v1
		std::vector<int> v2Neighbors(adjacency[v2].begin(), adjacency[v2].end());
		for (int neighbor : v2Neighbors)
		{
			if (neighbor == v1) continue;

			// Remove v2 from neighbor's list
			adjacency[neighbor].erase(v2);

			// Add v1 to neighbor's list and vice versa
			adjacency[neighbor].insert(v1);
			adjacency[v1].insert(neighbor);
		}

		// Remove v2 from v1's adjacency and clear v2's adjacency
		adjacency[v1].erase(v2);
		adjacency[v2].clear();

		// Add new edges to queue (v1's updated neighborhood)
		for (int neighbor : adjacency[v1])
		{
			if (!vertices[neighbor].bIsActive) continue;
			// Only add edges that are actually valid
			if (adjacency[neighbor].count(v1) > 0)
			{
				addEdgeToQueue(v1, neighbor);
			}
		}
	}

	// 5. Build simplified mesh
	FStaticMesh* simplifiedMesh = new FStaticMesh();
	std::unordered_map<int, int> oldToNewIndex;

	// Copy material info from original mesh
	simplifiedMesh->MaterialInfo = InOriginalMesh->MaterialInfo;

	// Add active vertices
	int newVertexIndex = 0;
	for (int i = 0; i < vertices.size(); ++i)
	{
		if (vertices[i].bIsActive)
		{
			FNormalVertex newVertex;
			newVertex.Position = vertices[i].Position;
			newVertex.Normal = vertices[i].Normal;
			newVertex.Color = vertices[i].Color;
			newVertex.TexCoord = vertices[i].TexCoord; // CRITICAL: Copy UV coordinates

			simplifiedMesh->Vertices.push_back(newVertex);
			oldToNewIndex[i] = newVertexIndex++;
		}
	}

	// Add active faces
	for (const auto& face : faces)
	{
		if (!face.bIsActive) continue;

		// Make sure all vertices are mapped
		if (oldToNewIndex.find(face.V[0]) != oldToNewIndex.end() &&
			oldToNewIndex.find(face.V[1]) != oldToNewIndex.end() &&
			oldToNewIndex.find(face.V[2]) != oldToNewIndex.end())
		{
			simplifiedMesh->Indices.push_back(oldToNewIndex[face.V[0]]);
			simplifiedMesh->Indices.push_back(oldToNewIndex[face.V[1]]);
			simplifiedMesh->Indices.push_back(oldToNewIndex[face.V[2]]);
		}
	}

	// Create mesh sections based on original mesh
	if (!simplifiedMesh->Indices.empty())
	{
		if (!InOriginalMesh->Sections.empty())
		{
			// Copy original sections structure (simplified mesh uses all indices in one section)
			for (const auto& originalSection : InOriginalMesh->Sections)
			{
				FMeshSection section;
				section.MaterialSlot = originalSection.MaterialSlot;
				section.StartIndex = 0; // Simplified mesh has all indices together
				section.IndexCount = simplifiedMesh->Indices.size();
				simplifiedMesh->Sections.push_back(section);
				break; // Use only the first section for now
			}
		}
		else
		{
			// Fallback: create default section
			FMeshSection section;
			section.MaterialSlot = 0;
			section.StartIndex = 0;
			section.IndexCount = simplifiedMesh->Indices.size();
			simplifiedMesh->Sections.push_back(section);
		}
	}

	return simplifiedMesh;
}
