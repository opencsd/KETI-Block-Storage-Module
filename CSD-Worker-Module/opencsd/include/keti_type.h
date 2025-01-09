typedef enum MySQL_DataType{
    MySQL_BYTE = 1,
    MySQL_INT16 = 2,
    MySQL_INT32 = 3,
    MySQL_INT64 = 8,
    MySQL_FLOAT32 = 4,
    MySQL_DOUBLE = 5,
    MySQL_NEWDECIMAL = 246,
    MySQL_DATE = 14,
    MySQL_TIMESTAMP = 7,
    MySQL_STRING = 254,
    MySQL_VARSTRING = 15,
}MySQL_DataType;

typedef enum KETI_SELECT_TYPE{
      COL_NAME = 0, //DEFAULT
      SUM = 1,
      AVG = 2,
      COUNT = 3,
      COUNTSTAR = 4,
      COUNTDISTINCT = 5,
      TOP = 6,
      MIN = 7,
      MAX = 8,
}KETI_SELECT_TYPE;

typedef enum KETI_VALUE_TYPE{
    INT8 = 0,
    INT16 = 1,
    INT32 = 2,
    INT64 = 3,
    FLOAT32 = 4,
    DOUBLE = 5,
    DECIMAL = 6,
    DATE = 7,
    TIMESTAMP = 8,
    STRING = 9,
    COLUMN = 10,
    OPERATOR = 11,
    BOOLEAN = 12
}KETI_VALUE_TYPE;

typedef enum KETI_OPER_TYPE{
    GE = 0,  // >=
    LE,      // <=
    GT,      // >
    LT,      // <
    EQ,      // ==
    NE,      // !=
    LIKE,  
    NOTLIKE,  
    BETWEEN, 
    IN,
    NOTIN,   
    IS,      
    ISNOT,   
    AND,     // AND --> 혼자 들어오는 oper
    OR,      // OR --> 혼자 들어오는 oper
}opertype;

enum SNIPPET_TYPE{
  FULL_SCAN = 0,
  INDEX_SCAN = 1,
  INDEX_TABLE_SCAN = 2,
  TMAX_SNIPPET = 15,
  SST_FILE_SCAN = 16
};