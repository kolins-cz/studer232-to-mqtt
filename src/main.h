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
    char *name;              // Technical ID: xt1_input_active_power
    char *friendly_name;     // Display name: Studer 1 Input Active Power
    char *mqtt_prefix;
    char *unit;
    int sign;
    char *device_class;      // Home Assistant device class
} parameter_t;

const char *mqtt_topic = "studer";

// Number of parameters in the array
#define NUM_PARAMETERS (sizeof(requested_parameters) / sizeof(parameter_t))

// List of parameters
// param, addr, name (ID), friendly_name, mqtt_prefix, unit, sign, device_class
const parameter_t requested_parameters[] = {
    {3137, 101, "xt1_input_active_power",     "Studer 1 Input Active Power",    "XT", "kW", -1, "power"},
    {3138, 101, "xt1_input_apparent_power",   "Studer 1 Input Apparent Power",  "XT", "kVA", 1, "apparent_power"},
    {3136, 101, "xt1_output_active_power",    "Studer 1 Output Active Power",   "XT", "kW", -1, "power"},
    {3139, 101, "xt1_output_apparent_power",  "Studer 1 Output Apparent Power", "XT", "kVA", 1, "apparent_power"},
    {3137, 102, "xt2_input_active_power",     "Studer 2 Input Active Power",    "XT", "kW", -1, "power"},
    {3138, 102, "xt2_input_apparent_power",   "Studer 2 Input Apparent Power",  "XT", "kVA", 1, "apparent_power"},
    {3136, 102, "xt2_output_active_power",    "Studer 2 Output Active Power",   "XT", "kW", -1, "power"},
    {3139, 102, "xt2_output_apparent_power",  "Studer 2 Output Apparent Power", "XT", "kVA", 1, "apparent_power"},
    {3137, 103, "xt3_input_active_power",     "Studer 3 Input Active Power",    "XT", "kW", -1, "power"},
    {3138, 103, "xt3_input_apparent_power",   "Studer 3 Input Apparent Power",  "XT", "kVA", 1, "apparent_power"},
    {3136, 103, "xt3_output_active_power",    "Studer 3 Output Active Power",   "XT", "kW", -1, "power"},
    {3139, 103, "xt3_output_apparent_power",  "Studer 3 Output Apparent Power", "XT", "kVA", 1, "apparent_power"},
    {3137, 104, "xt4_input_active_power",     "Studer 4 Input Active Power",    "XT", "kW", -1, "power"},
    {3138, 104, "xt4_input_apparent_power",   "Studer 4 Input Apparent Power",  "XT", "kVA", 1, "apparent_power"},
    {3136, 104, "xt4_output_active_power",    "Studer 4 Output Active Power",   "XT", "kW", -1, "power"},
    {3139, 104, "xt4_output_apparent_power",  "Studer 4 Output Apparent Power", "XT", "kVA", 1, "apparent_power"},
    {3137, 191, "l1_input_active_power",      "Studer L1 Input Active Power",    "AC", "kW", -1, "power"},
    {3138, 191, "l1_input_apparent_power",    "Studer L1 Input Apparent Power",  "AC", "kVA", 1, "apparent_power"},
    {3136, 191, "l1_output_active_power",     "Studer L1 Output Active Power",   "AC", "kW", -1, "power"},
    {3139, 191, "l1_output_apparent_power",   "Studer L1 Output Apparent Power", "AC", "kVA", 1, "apparent_power"},
    {3137, 192, "l2_input_active_power",      "Studer L2 Input Active Power",    "AC", "kW", -1, "power"},
    {3138, 192, "l2_input_apparent_power",    "Studer L2 Input Apparent Power",  "AC", "kVA", 1, "apparent_power"},
    {3136, 192, "l2_output_active_power",     "Studer L2 Output Active Power",   "AC", "kW", -1, "power"},
    {3139, 192, "l2_output_apparent_power",   "Studer L2 Output Apparent Power", "AC", "kVA", 1, "apparent_power"},
    {3137, 193, "l3_input_active_power",      "Studer L3 Input Active Power",    "AC", "kW", -1, "power"},
    {3138, 193, "l3_input_apparent_power",    "Studer L3 Input Apparent Power",  "AC", "kVA", 1, "apparent_power"},
    {3136, 193, "l3_output_active_power",     "Studer L3 Output Active Power",   "AC", "kW", -1, "power"},
    {3139, 193, "l3_output_apparent_power",   "Studer L3 Output Apparent Power", "AC", "kVA", 1, "apparent_power"},
    {3104, 101, "xt1_temperature",            "Studer 1 Temperature",            "XT", "°C",  1, "temperature"},
    {3104, 102, "xt2_temperature",            "Studer 2 Temperature",            "XT", "°C",  1, "temperature"},
    {3104, 103, "xt3_temperature",            "Studer 3 Temperature",            "XT", "°C",  1, "temperature"},
    {3104, 104, "xt4_temperature",            "Studer 4 Temperature",            "XT", "°C",  1, "temperature"},
    {3085, 100, "output_freq",                "Studer AC Output Frequency",      "AC", "Hz",  1, "frequency"},
    {3137, 100, "total_input_active_power",   "Studer AC Total Input Active Power",  "AC", "kW", -1, "power"},
    {3136, 100, "total_output_active_power",  "Studer AC Total Output Active Power", "AC", "kW", -1, "power"},
    {3000, 100, "batt_voltage",               "Studer DC Battery Voltage",       "DC", "V",   1, "voltage"},
    {3005, 191, "l1_batt_current",            "Studer L1 Battery Current",       "DC", "A",   1, "current"},
    {3005, 192, "l2_batt_current",            "Studer L2 Battery Current",       "DC", "A",   1, "current"},
    {3005, 193, "l3_batt_current",            "Studer L3 Battery Current",       "DC", "A",   1, "current"},
    {3005, 101, "xt1_batt_current",           "Studer 1 Battery Current",        "DC", "A",   1, "current"},
    {3005, 102, "xt2_batt_current",           "Studer 2 Battery Current",        "DC", "A",   1, "current"},
    {3005, 103, "xt3_batt_current",           "Studer 3 Battery Current",        "DC", "A",   1, "current"},
    {3005, 104, "xt4_batt_current",           "Studer 4 Battery Current",        "DC", "A",   1, "current"},
};

