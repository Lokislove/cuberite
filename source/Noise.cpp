
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "Noise.h"





#if NOISE_USE_SSE
	#include <smmintrin.h> //_mm_mul_epi32
#endif

#define FAST_FLOOR(x) (((x) < 0) ? (((int)x) - 1) : ((int)x))





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:

void IntArrayLinearInterpolate2D(
	int * a_Array, 
	int a_SizeX, int a_SizeY,  // Dimensions of the array
	int a_AnchorStepX, int a_AnchorStepY  // Distances between the anchor points in each direction
)
{
	// First interpolate columns where the anchor points are:
	int LastYCell = a_SizeY - a_AnchorStepY;
	for (int y = 0; y < LastYCell; y += a_AnchorStepY)
	{
		int Idx = a_SizeX * y;
		for (int x = 0; x < a_SizeX; x += a_AnchorStepX)
		{
			int StartValue = a_Array[Idx];
			int EndValue   = a_Array[Idx + a_SizeX * a_AnchorStepY];
			int Diff = EndValue - StartValue;
			for (int CellY = 1; CellY < a_AnchorStepY; CellY++)
			{
				a_Array[Idx + a_SizeX * CellY] = StartValue + CellY * Diff / a_AnchorStepY;
			}  // for CellY
			Idx += a_AnchorStepX;
		}  // for x
	}  // for y
	
	// Now interpolate in rows, each row has values in the anchor columns
	int LastXCell = a_SizeX - a_AnchorStepX;
	for (int y = 0; y < a_SizeY; y++)
	{
		int Idx = a_SizeX * y;
		for (int x = 0; x < LastXCell; x += a_AnchorStepX)
		{
			int StartValue = a_Array[Idx];
			int EndValue   = a_Array[Idx + a_AnchorStepX];
			int Diff = EndValue - StartValue;
			for (int CellX = 1; CellX < a_AnchorStepX; CellX++)
			{
				a_Array[Idx + CellX] = StartValue + CellX * Diff / a_AnchorStepX;
			}  // for CellY
			Idx += a_AnchorStepX;
		}
	}
}





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cCubicCell2D:

class cCubicCell2D
{
public:
	cCubicCell2D(
		cNoise & a_Noise,          ///< Noise to use for generating the random values
		NOISE_DATATYPE * a_Array,  ///< Array to generate into [x + a_SizeX * y]
		int a_SizeX, int a_SizeY,  ///< Count of the array, in each direction
		const NOISE_DATATYPE * a_FracX,  ///< Pointer to the array that stores the X fractional values
		const NOISE_DATATYPE * a_FracY   ///< Pointer to the attay that stores the Y fractional values
	);
	
	/// Uses current m_WorkRnds[] to generate part of the array
	void Generate(
		int a_FromX, int a_ToX,
		int a_FromY, int a_ToY
	);
	
	/// Initializes m_WorkRnds[] with the specified Floor values
	void InitWorkRnds(int a_FloorX, int a_FloorY);
	
	/// Updates m_WorkRnds[] for the new Floor values.
	void Move(int a_NewFloorX, int a_NewFloorY);

protected:
	typedef NOISE_DATATYPE Workspace[4][4];
	
	cNoise & m_Noise;
	
	Workspace * m_WorkRnds;  ///< The current random values; points to either m_Workspace1 or m_Workspace2 (doublebuffering)
	Workspace m_Workspace1;  ///< Buffer 1 for workspace doublebuffering, used in Move()
	Workspace m_Workspace2;  ///< Buffer 2 for workspace doublebuffering, used in Move()
	int m_CurFloorX;
	int m_CurFloorY;
	
	NOISE_DATATYPE * m_Array;
	int m_SizeX, m_SizeY;
	const NOISE_DATATYPE * m_FracX;
	const NOISE_DATATYPE * m_FracY;
} ;





cCubicCell2D::cCubicCell2D(
	cNoise & a_Noise,          ///< Noise to use for generating the random values
	NOISE_DATATYPE * a_Array,  ///< Array to generate into [x + a_SizeX * y]
	int a_SizeX, int a_SizeY,  ///< Count of the array, in each direction
	const NOISE_DATATYPE * a_FracX,  ///< Pointer to the array that stores the X fractional values
	const NOISE_DATATYPE * a_FracY   ///< Pointer to the attay that stores the Y fractional values
) :
	m_Noise(a_Noise),
	m_WorkRnds(&m_Workspace1),
	m_Array(a_Array),
	m_SizeX(a_SizeX),
	m_SizeY(a_SizeY),
	m_FracX(a_FracX),
	m_FracY(a_FracY)
{
}





void cCubicCell2D::Generate(
	int a_FromX, int a_ToX,
	int a_FromY, int a_ToY
)
{
	for (int y = a_FromY; y < a_ToY; y++)
	{
		NOISE_DATATYPE Interp[4];
		NOISE_DATATYPE FracY = m_FracY[y];
		Interp[0] = cNoise::CubicInterpolate((*m_WorkRnds)[0][0], (*m_WorkRnds)[0][1], (*m_WorkRnds)[0][2], (*m_WorkRnds)[0][3], FracY);
		Interp[1] = cNoise::CubicInterpolate((*m_WorkRnds)[1][0], (*m_WorkRnds)[1][1], (*m_WorkRnds)[1][2], (*m_WorkRnds)[1][3], FracY);
		Interp[2] = cNoise::CubicInterpolate((*m_WorkRnds)[2][0], (*m_WorkRnds)[2][1], (*m_WorkRnds)[2][2], (*m_WorkRnds)[2][3], FracY);
		Interp[3] = cNoise::CubicInterpolate((*m_WorkRnds)[3][0], (*m_WorkRnds)[3][1], (*m_WorkRnds)[3][2], (*m_WorkRnds)[3][3], FracY);
		int idx = y * m_SizeX + a_FromX;
		for (int x = a_FromX; x < a_ToX; x++)
		{
			m_Array[idx++] = cNoise::CubicInterpolate(Interp[0], Interp[1], Interp[2], Interp[3], m_FracX[x]);
		}  // for x
	}  // for y
}





void cCubicCell2D::InitWorkRnds(int a_FloorX, int a_FloorY)
{
	m_CurFloorX = a_FloorX;
	m_CurFloorY = a_FloorY;
	for (int x = 0; x < 4; x++)
	{
		int cx = a_FloorX + x - 1;
		for (int y = 0; y < 4; y++)
		{
			int cy = a_FloorY + y - 1;
			(*m_WorkRnds)[x][y] = (NOISE_DATATYPE)m_Noise.IntNoise2D(cx, cy);
		}
	}
}





void cCubicCell2D::Move(int a_NewFloorX, int a_NewFloorY)
{
	// Swap the doublebuffer:
	int OldFloorX = m_CurFloorX;
	int OldFloorY = m_CurFloorY;
	Workspace * OldWorkRnds = m_WorkRnds;
	m_WorkRnds = (m_WorkRnds == &m_Workspace1) ? &m_Workspace2 : &m_Workspace1;
	
	// Reuse as much of the old workspace as possible:
	int DiffX = OldFloorX - a_NewFloorX;
	int DiffY = OldFloorY - a_NewFloorY;
	for (int x = 0; x < 4; x++)
	{
		int cx = a_NewFloorX + x - 1;
		int OldX = x - DiffX;  // Where would this X be in the old grid?
		for (int y = 0; y < 4; y++)
		{
			int cy = a_NewFloorY + y - 1;
			int OldY = y - DiffY;  // Where would this Y be in the old grid?
			if ((OldX >= 0) && (OldX < 4) && (OldY >= 0) && (OldY < 4))
			{
				(*m_WorkRnds)[x][y] = (*OldWorkRnds)[OldX][OldY];
			}
			else
			{
				(*m_WorkRnds)[x][y] = (NOISE_DATATYPE)m_Noise.IntNoise2D(cx, cy);
			}
		}
	}
	m_CurFloorX = a_NewFloorX;
	m_CurFloorY = a_NewFloorY;
}





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cNoise:

cNoise::cNoise(unsigned int a_Seed) :
	m_Seed(a_Seed)
{
}





NOISE_DATATYPE cNoise::LinearNoise1D(NOISE_DATATYPE a_X) const
{
	int BaseX = FAST_FLOOR(a_X);
	NOISE_DATATYPE FracX = a_X - BaseX;
	return LinearInterpolate(IntNoise1D(BaseX), IntNoise1D(BaseX + 1), FracX);
}





NOISE_DATATYPE cNoise::CosineNoise1D(NOISE_DATATYPE a_X) const
{
	int BaseX = FAST_FLOOR(a_X);
	NOISE_DATATYPE FracX = a_X - BaseX;
	return CosineInterpolate(IntNoise1D(BaseX), IntNoise1D(BaseX + 1), FracX);
}





NOISE_DATATYPE cNoise::CubicNoise1D(NOISE_DATATYPE a_X) const
{
	int BaseX = FAST_FLOOR(a_X);
	NOISE_DATATYPE FracX = a_X - BaseX;
	return CubicInterpolate(IntNoise1D(BaseX - 1), IntNoise1D(BaseX), IntNoise1D(BaseX + 1), IntNoise1D(BaseX + 2), FracX);
}





NOISE_DATATYPE cNoise::SmoothNoise1D(int a_X) const
{
	return IntNoise1D(a_X) / 2 + IntNoise1D(a_X - 1) / 4 + IntNoise1D(a_X + 1) / 4;
}





NOISE_DATATYPE cNoise::CubicNoise2D(NOISE_DATATYPE a_X, NOISE_DATATYPE a_Y) const
{
	const int	BaseX = FAST_FLOOR(a_X);
	const int	BaseY = FAST_FLOOR(a_Y);
	
	const NOISE_DATATYPE points[4][4] =
	{
		IntNoise2D(BaseX - 1, BaseY - 1), IntNoise2D(BaseX, BaseY - 1), IntNoise2D(BaseX + 1, BaseY - 1), IntNoise2D(BaseX + 2, BaseY - 1),
		IntNoise2D(BaseX - 1, BaseY),     IntNoise2D(BaseX, BaseY),     IntNoise2D(BaseX + 1, BaseY),     IntNoise2D(BaseX + 2, BaseY),
		IntNoise2D(BaseX - 1, BaseY + 1), IntNoise2D(BaseX, BaseY + 1), IntNoise2D(BaseX + 1, BaseY + 1), IntNoise2D(BaseX + 2, BaseY + 1),
		IntNoise2D(BaseX - 1, BaseY + 2), IntNoise2D(BaseX, BaseY + 2), IntNoise2D(BaseX + 1, BaseY + 2), IntNoise2D(BaseX + 2, BaseY + 2),
	};

	const NOISE_DATATYPE FracX = a_X - BaseX;
	const NOISE_DATATYPE interp1 = CubicInterpolate(points[0][0], points[0][1], points[0][2], points[0][3], FracX);
	const NOISE_DATATYPE interp2 = CubicInterpolate(points[1][0], points[1][1], points[1][2], points[1][3], FracX);
	const NOISE_DATATYPE interp3 = CubicInterpolate(points[2][0], points[2][1], points[2][2], points[2][3], FracX);
	const NOISE_DATATYPE interp4 = CubicInterpolate(points[3][0], points[3][1], points[3][2], points[3][3], FracX);


	const NOISE_DATATYPE FracY = a_Y - BaseY;
	return CubicInterpolate(interp1, interp2, interp3, interp4, FracY);
}





NOISE_DATATYPE cNoise::CubicNoise3D(NOISE_DATATYPE a_X, NOISE_DATATYPE a_Y, NOISE_DATATYPE a_Z) const
{
	const int	BaseX = FAST_FLOOR(a_X);
	const int	BaseY = FAST_FLOOR(a_Y);
	const int	BaseZ = FAST_FLOOR(a_Z);
	
	const NOISE_DATATYPE points1[4][4] = { 
		IntNoise3D(BaseX - 1, BaseY - 1, BaseZ - 1), IntNoise3D(BaseX, BaseY - 1, BaseZ - 1), IntNoise3D(BaseX + 1, BaseY - 1, BaseZ - 1), IntNoise3D(BaseX + 2, BaseY - 1, BaseZ - 1),
		IntNoise3D(BaseX - 1, BaseY,     BaseZ - 1), IntNoise3D(BaseX, BaseY,     BaseZ - 1), IntNoise3D(BaseX + 1, BaseY,     BaseZ - 1), IntNoise3D(BaseX + 2, BaseY,     BaseZ - 1),
		IntNoise3D(BaseX - 1, BaseY + 1, BaseZ - 1), IntNoise3D(BaseX, BaseY + 1, BaseZ - 1), IntNoise3D(BaseX + 1, BaseY + 1, BaseZ - 1), IntNoise3D(BaseX + 2, BaseY + 1, BaseZ - 1),
		IntNoise3D(BaseX - 1, BaseY + 2, BaseZ - 1), IntNoise3D(BaseX, BaseY + 2, BaseZ - 1), IntNoise3D(BaseX + 1, BaseY + 2, BaseZ - 1), IntNoise3D(BaseX + 2, BaseY + 2, BaseZ - 1),
	};

	const NOISE_DATATYPE	FracX = (a_X) - BaseX;
	const NOISE_DATATYPE x1interp1 = CubicInterpolate( points1[0][0], points1[0][1], points1[0][2], points1[0][3], FracX );
	const NOISE_DATATYPE x1interp2 = CubicInterpolate( points1[1][0], points1[1][1], points1[1][2], points1[1][3], FracX );
	const NOISE_DATATYPE x1interp3 = CubicInterpolate( points1[2][0], points1[2][1], points1[2][2], points1[2][3], FracX );
	const NOISE_DATATYPE x1interp4 = CubicInterpolate( points1[3][0], points1[3][1], points1[3][2], points1[3][3], FracX );

	const NOISE_DATATYPE points2[4][4] = { 
		IntNoise3D( BaseX-1, BaseY-1, BaseZ ), IntNoise3D( BaseX, BaseY-1, BaseZ ),	IntNoise3D( BaseX+1, BaseY-1, BaseZ ), IntNoise3D( BaseX+2, BaseY-1, BaseZ ),
		IntNoise3D( BaseX-1, BaseY,	  BaseZ ), IntNoise3D( BaseX, BaseY,   BaseZ ),	IntNoise3D( BaseX+1, BaseY,   BaseZ ), IntNoise3D( BaseX+2, BaseY,   BaseZ ),
		IntNoise3D( BaseX-1, BaseY+1, BaseZ ), IntNoise3D( BaseX, BaseY+1, BaseZ ),	IntNoise3D( BaseX+1, BaseY+1, BaseZ ), IntNoise3D( BaseX+2, BaseY+1, BaseZ ),
		IntNoise3D( BaseX-1, BaseY+2, BaseZ ), IntNoise3D( BaseX, BaseY+2, BaseZ ),	IntNoise3D( BaseX+1, BaseY+2, BaseZ ), IntNoise3D( BaseX+2, BaseY+2, BaseZ ),
	};

	const NOISE_DATATYPE x2interp1 = CubicInterpolate( points2[0][0], points2[0][1], points2[0][2], points2[0][3], FracX );
	const NOISE_DATATYPE x2interp2 = CubicInterpolate( points2[1][0], points2[1][1], points2[1][2], points2[1][3], FracX );
	const NOISE_DATATYPE x2interp3 = CubicInterpolate( points2[2][0], points2[2][1], points2[2][2], points2[2][3], FracX );
	const NOISE_DATATYPE x2interp4 = CubicInterpolate( points2[3][0], points2[3][1], points2[3][2], points2[3][3], FracX );

	const NOISE_DATATYPE points3[4][4] = { 
		IntNoise3D( BaseX-1, BaseY-1, BaseZ+1 ), IntNoise3D( BaseX, BaseY-1, BaseZ+1 ),	IntNoise3D( BaseX+1, BaseY-1, BaseZ+1 ), IntNoise3D( BaseX+2, BaseY-1, BaseZ+1 ),
		IntNoise3D( BaseX-1, BaseY,	  BaseZ+1 ), IntNoise3D( BaseX, BaseY,   BaseZ+1 ),	IntNoise3D( BaseX+1, BaseY,   BaseZ+1 ), IntNoise3D( BaseX+2, BaseY,   BaseZ+1 ),
		IntNoise3D( BaseX-1, BaseY+1, BaseZ+1 ), IntNoise3D( BaseX, BaseY+1, BaseZ+1 ),	IntNoise3D( BaseX+1, BaseY+1, BaseZ+1 ), IntNoise3D( BaseX+2, BaseY+1, BaseZ+1 ),
		IntNoise3D( BaseX-1, BaseY+2, BaseZ+1 ), IntNoise3D( BaseX, BaseY+2, BaseZ+1 ),	IntNoise3D( BaseX+1, BaseY+2, BaseZ+1 ), IntNoise3D( BaseX+2, BaseY+2, BaseZ+1 ),
	};

	const NOISE_DATATYPE x3interp1 = CubicInterpolate( points3[0][0], points3[0][1], points3[0][2], points3[0][3], FracX );
	const NOISE_DATATYPE x3interp2 = CubicInterpolate( points3[1][0], points3[1][1], points3[1][2], points3[1][3], FracX );
	const NOISE_DATATYPE x3interp3 = CubicInterpolate( points3[2][0], points3[2][1], points3[2][2], points3[2][3], FracX );
	const NOISE_DATATYPE x3interp4 = CubicInterpolate( points3[3][0], points3[3][1], points3[3][2], points3[3][3], FracX );

	const NOISE_DATATYPE points4[4][4] = { 
		IntNoise3D( BaseX-1, BaseY-1, BaseZ+2 ), IntNoise3D( BaseX, BaseY-1, BaseZ+2 ),	IntNoise3D( BaseX+1, BaseY-1, BaseZ+2 ), IntNoise3D( BaseX+2, BaseY-1, BaseZ+2 ),
		IntNoise3D( BaseX-1, BaseY,	  BaseZ+2 ), IntNoise3D( BaseX, BaseY,   BaseZ+2 ),	IntNoise3D( BaseX+1, BaseY,   BaseZ+2 ), IntNoise3D( BaseX+2, BaseY,   BaseZ+2 ),
		IntNoise3D( BaseX-1, BaseY+1, BaseZ+2 ), IntNoise3D( BaseX, BaseY+1, BaseZ+2 ),	IntNoise3D( BaseX+1, BaseY+1, BaseZ+2 ), IntNoise3D( BaseX+2, BaseY+1, BaseZ+2 ),
		IntNoise3D( BaseX-1, BaseY+2, BaseZ+2 ), IntNoise3D( BaseX, BaseY+2, BaseZ+2 ),	IntNoise3D( BaseX+1, BaseY+2, BaseZ+2 ), IntNoise3D( BaseX+2, BaseY+2, BaseZ+2 ),
	};

	const NOISE_DATATYPE x4interp1 = CubicInterpolate( points4[0][0], points4[0][1], points4[0][2], points4[0][3], FracX );
	const NOISE_DATATYPE x4interp2 = CubicInterpolate( points4[1][0], points4[1][1], points4[1][2], points4[1][3], FracX );
	const NOISE_DATATYPE x4interp3 = CubicInterpolate( points4[2][0], points4[2][1], points4[2][2], points4[2][3], FracX );
	const NOISE_DATATYPE x4interp4 = CubicInterpolate( points4[3][0], points4[3][1], points4[3][2], points4[3][3], FracX );

	const NOISE_DATATYPE	FracY = (a_Y) - BaseY;
	const NOISE_DATATYPE yinterp1 = CubicInterpolate( x1interp1, x1interp2, x1interp3, x1interp4, FracY );
	const NOISE_DATATYPE yinterp2 = CubicInterpolate( x2interp1, x2interp2, x2interp3, x2interp4, FracY );
	const NOISE_DATATYPE yinterp3 = CubicInterpolate( x3interp1, x3interp2, x3interp3, x3interp4, FracY );
	const NOISE_DATATYPE yinterp4 = CubicInterpolate( x4interp1, x4interp2, x4interp3, x4interp4, FracY );

	const NOISE_DATATYPE	FracZ = (a_Z) - BaseZ;
	return CubicInterpolate( yinterp1, yinterp2, yinterp3, yinterp4, FracZ );
}





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cCubicNoise:

cCubicNoise::cCubicNoise(int a_Seed) :
	m_Noise(a_Seed)
{
}





void cCubicNoise::Generate2D(
	NOISE_DATATYPE * a_Array,                        ///< Array to generate into [x + a_SizeX * y]
	int a_SizeX, int a_SizeY,                        ///< Size of the array (num doubles), in each direction
	NOISE_DATATYPE a_StartX, NOISE_DATATYPE a_EndX,  ///< Noise-space coords of the array in the X direction
	NOISE_DATATYPE a_StartY, NOISE_DATATYPE a_EndY,  ///< Noise-space coords of the array in the Y direction
	NOISE_DATATYPE * a_Workspace                     ///< Workspace that this function can use and trash
)
{
	ASSERT(a_SizeX < MAX_SIZE);
	ASSERT(a_SizeY < MAX_SIZE);
	ASSERT(a_StartX < a_EndX);
	ASSERT(a_StartY < a_EndY);
	
	// Calculate the integral and fractional parts of each coord:
	int FloorX[MAX_SIZE];
	int FloorY[MAX_SIZE];
	NOISE_DATATYPE FracX[MAX_SIZE];
	NOISE_DATATYPE FracY[MAX_SIZE];
	int SameX[MAX_SIZE];
	int SameY[MAX_SIZE];
	int NumSameX, NumSameY;
	CalcFloorFrac(a_SizeX, a_StartX, a_EndX, FloorX, FracX, SameX, NumSameX);
	CalcFloorFrac(a_SizeY, a_StartY, a_EndY, FloorY, FracY, SameY, NumSameY);
	
	cCubicCell2D Cell(m_Noise, a_Array, a_SizeX, a_SizeY, FracX, FracY);
	
	Cell.InitWorkRnds(FloorX[0], FloorY[0]);
	
	// Calculate query values using Cell:
	int FromY = 0;
	for (int y = 0; y < NumSameY; y++)
	{
		int ToY = FromY + SameY[y];
		int FromX = 0;
		int CurFloorY = FloorY[FromY];
		for (int x = 0; x < NumSameX; x++)
		{
			int ToX = FromX + SameX[x];
			Cell.Generate(FromX, ToX, FromY, ToY);
			Cell.Move(FloorX[ToX], CurFloorY);
			FromX = ToX;
		}
		Cell.Move(FloorX[0], FloorY[ToY]);
		FromY = ToY;
	}
}





void cCubicNoise::CalcFloorFrac(
	int a_Size,
	NOISE_DATATYPE a_Start, NOISE_DATATYPE a_End,
	int * a_Floor, NOISE_DATATYPE * a_Frac,
	int * a_Same, int & a_NumSame
)
{
	NOISE_DATATYPE val = a_Start;
	NOISE_DATATYPE dif = (a_End - a_Start) / a_Size;
	for (int i = 0; i < a_Size; i++)
	{
		a_Floor[i] = FAST_FLOOR(val);
		a_Frac[i] = val - a_Floor[i];
		val += dif;
	}
	
	// Mark up the same floor values into a_Same / a_NumSame:
	int CurFloor = a_Floor[0];
	int LastSame = 0;
	a_NumSame = 0;
	for (int i = 1; i < a_Size; i++)
	{
		if (a_Floor[i] != CurFloor)
		{
			a_Same[a_NumSame] = i - LastSame;
			LastSame = i;
			a_NumSame += 1;
			CurFloor = a_Floor[i];
		}
	}  // for i - a_Floor[]
	if (LastSame < a_Size)
	{
		a_Same[a_NumSame] = a_Size - LastSame;
		a_NumSame += 1;
	}
}




