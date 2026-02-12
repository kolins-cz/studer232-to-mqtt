const char *mqtt_server = "net.ad.kolins.cz";
int mqtt_port = 1883;

const char *lwt_message = "offline";

// Structure to hold the result of reading a parameter
typedef struct {
    float value; // Value of the parameter
    int error;   // Error code (0 if no error)
} read_param_result_t;

typedef struct {
    int parameter;
    int address;
    char *name;
    char *mqtt_prefix;
    char *unit;
    int sign;
} parameter_t;

const char *mqtt_topic = "studer";

// Number of parameters in the array
#define NUM_PARAMETERS (sizeof(requested_parameters) / sizeof(parameter_t))

// List of parameters
// param,	addr,	name,	mqtt prefix,	unit, sign
const parameter_t requested_parameters[] = {
    {3137, 101, "xt1_input_active_power",     "XT", "kW", -1},
    {3138, 101, "xt1_input_apparent_power",   "XT", "kVA", 1},
    {3136, 101, "xt1_output_active_power",    "XT", "kW", -1},
    {3139, 101, "xt1_output_apparent_power",  "XT", "kVA", 1},
    {3137, 102, "xt2_input_active_power",     "XT", "kW", -1},
    {3138, 102, "xt2_input_apparent_power",   "XT", "kVA", 1},
    {3136, 102, "xt2_output_active_power",    "XT", "kW", -1},
    {3139, 102, "xt2_output_apparent_power",  "XT", "kVA", 1},
    {3137, 103, "xt3_input_active_power",     "XT", "kW", -1},
    {3138, 103, "xt3_input_apparent_power",   "XT", "kVA", 1},
    {3136, 103, "xt3_output_active_power",    "XT", "kW", -1},
    {3139, 103, "xt3_output_apparent_power",  "XT", "kVA", 1},
    {3137, 104, "xt4_input_active_power",     "XT", "kW", -1},
    {3138, 104, "xt4_input_apparent_power",   "XT", "kVA", 1},
    {3136, 104, "xt4_output_active_power",    "XT", "kW", -1},
    {3139, 104, "xt4_output_apparent_power",  "XT", "kVA", 1},
    {3137, 191, "l1_input_active_power",      "AC", "kW", -1},
    {3138, 191, "l1_input_apparent_power",    "AC", "kVA", 1},
    {3136, 191, "l1_output_active_power",     "AC", "kW", -1},
    {3139, 191, "l1_output_apparent_power",   "AC", "kVA", 1},
    {3137, 192, "l2_input_active_power",      "AC", "kW", -1},
    {3138, 192, "l2_input_apparent_power",    "AC", "kVA", 1},
    {3136, 192, "l2_output_active_power",     "AC", "kW", -1},
    {3139, 192, "l2_output_apparent_power",   "AC", "kVA", 1},
    {3137, 193, "l3_input_active_power",      "AC", "kW", -1},
    {3138, 193, "l3_input_apparent_power",    "AC", "kVA", 1},
    {3136, 193, "l3_output_active_power",     "AC", "kW", -1},
    {3139, 193, "l3_output_apparent_power",   "AC", "kVA", 1},
    {3104, 101, "xt1_temperature",            "XT", "°C",  1},
    {3104, 102, "xt2_temperature",            "XT", "°C",  1},
    {3104, 103, "xt3_temperature",            "XT", "°C",  1},
    {3104, 104, "xt4_temperature",            "XT", "°C",  1},
    {3085, 100, "output_freq",                "AC", "Hz",  1},
    {3137, 100, "total_input_active_power",   "AC", "kW", -1},
    {3136, 100, "total_output_active_power",  "AC", "kW", -1},
    {3000, 100, "battery_voltage",            "DC", "V",   1},
    {3005, 191, "l1_batt_current",            "DC", "A",   1},
    {3005, 192, "l2_batt_current",            "DC", "A",   1},
    {3005, 193, "l3_batt_current",            "DC", "A",   1},
    {3005, 101, "xt1_batt_current",           "DC", "A",   1},
    {3005, 102, "xt2_batt_current",           "DC", "A",   1},
    {3005, 103, "xt3_batt_current",           "DC", "A",   1},
    {3005, 104, "xt4_batt_current",           "DC", "A",   1},
};

