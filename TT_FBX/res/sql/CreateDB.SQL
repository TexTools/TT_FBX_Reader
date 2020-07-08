-- Meta table for storing information such as 
-- Warning or Error codes, etc.
CREATE TABLE "meta" (
	"key" TEXT NOT NULL UNIQUE,
	"value" TEXT,
	PRIMARY KEY("key")
);

-- The Triangle Indices
CREATE TABLE "indices" (
	"mesh"	INTEGER NOT NULL,
	"part"   INTEGER NOT NULL,
	"index_id"	INTEGER NOT NULL,
	"vertex_id"		TEXT NOT NULL,
	

	PRIMARY KEY("mesh","part","index_id")
);


-- Vertex Data
CREATE TABLE "vertexes" (
	"vertex_id"	INTEGER NOT NULL,
	"mesh"	INTEGER NOT NULL,
	"part"   INTEGER NOT NULL,

	-- Position
	"pos_x"	REAL NOT NULL,
	"pos_y"	REAL NOT NULL,
	"pos_z"	REAL NOT NULL,

	-- Normal
	"normal_x"	REAL NOT NULL,
	"normal_y"  REAL NOT NULL,
	"normal_z"	REAL NOT NULL,
	
	-- Vertex Color
	"color_r"	REAL NOT NULL,
	"color_g"	REAL NOT NULL,
	"color_b"	REAL NOT NULL,
	"color_a"	REAL NOT NULL,

	-- UV Coordinates
	"uv_1_u"	REAL NOT NULL,
	"uv_y_v"	REAL NOT NULL,
	"uv_2_u"	REAL NOT NULL,
	"uv_2_v"	REAL NOT NULL,

	-- Bone Weights
	"bone_1_id"			INTEGER,
	"bone_1_weight"		REAL,
	"bone_2_id"			INTEGER,
	"bone_2_weight"		REAL,
	"bone_3_id"			INTEGER,
	"bone_3_weight"		REAL,
	"bone_4_id"			INTEGER,
	"bone_4_weight"		REAL,

	PRIMARY KEY("mesh","part","vertex_id")
);

-- Bones
CREATE TABLE "bones" (
	"bone_id"	INTEGER NOT NULL,
	"name"  TEXT NOT NULL,
	
	-- Format for storing bone matrices tbd.

	Primary KEY("bone_id")
);

-- Materials
CREATE TABLE "materials" (
	"mesh"			INTEGER NOT NULL,
	"part"			INTEGER NOT NULL,
	"usage"			TEXT NOT NULL,
	"path"			TEXT NOT NULL,

	Primary KEY("mesh","part","usage")
);