#include "lwip_socket.h"
#include "lwip_adin1110.h"

#ifndef DISABLE_SECURE_SOCKET
#define CA_CERT                                                             \
    "-----BEGIN CERTIFICATE-----\r\n"                                       \
    "MIICeDCCAh+gAwIBAgIUKVNcR24VCzYSKb6sKQbNF75eOm0wCgYIKoZIzj0EAwIw\r\n"  \
    "gZExCzAJBgNVBAYTAklFMRAwDgYDVQQIDAdtdW5zdGVyMREwDwYDVQQHDAhsaW1l\r\n"  \
    "cmljazEMMAoGA1UECgwDYWRpMREwDwYDVQQLDAhzb2Z0d2FyZTEVMBMGA1UEAwwM\r\n"  \
    "MTkyLjE2OC4wLjI5MSUwIwYJKoZIhvcNAQkBFhZzYWRoYmgucnlhbkBhbmFsb2cu\r\n"  \
    "Y29tMB4XDTI2MDUwNjEzNTcxMFoXDTM2MDUwMzEzNTcxMFowgZExCzAJBgNVBAYT\r\n"  \
    "AklFMRAwDgYDVQQIDAdtdW5zdGVyMREwDwYDVQQHDAhsaW1lcmljazEMMAoGA1UE\r\n"  \
    "CgwDYWRpMREwDwYDVQQLDAhzb2Z0d2FyZTEVMBMGA1UEAwwMMTkyLjE2OC4wLjI5\r\n"  \
    "MSUwIwYJKoZIhvcNAQkBFhZzYWRoYmgucnlhbkBhbmFsb2cuY29tMFkwEwYHKoZI\r\n"  \
    "zj0CAQYIKoZIzj0DAQcDQgAEOBjfgFmE5Me1pQKsZtYkh7gP+coZC4ZPI89ghSz1\r\n"  \
    "A/ooIRIcglVbM9JnC8nCF7HZHx4LRVN47vWJCloLbfA23qNTMFEwHQYDVR0OBBYE\r\n"  \
    "FO+VzGC7W1YDzCzXhVMD3I5Aw+LVMB8GA1UdIwQYMBaAFO+VzGC7W1YDzCzXhVMD\r\n"  \
    "3I5Aw+LVMA8GA1UdEwEB/wQFMAMBAf8wCgYIKoZIzj0EAwIDRwAwRAIgBRCyRNMl\r\n"  \
    "0VQmdGnvMMCLUH5P/5SsGUNxqQS8wVK0qYsCIDnNX4LZ9nY914AOUChDMrKHc1RF\r\n"  \
    "z4rFdk0TUtMM88vg\r\n"                                                  \
    "-----END CERTIFICATE-----\r\n"

#define DEVICE_CERT                                                         \
    "-----BEGIN CERTIFICATE-----\r\n"                                       \
    "MIICaTCCAg6gAwIBAgIUGo6O/zKI0WoZCGmr4+mh0X0D22YwCgYIKoZIzj0EAwIw\r\n"  \
    "gZExCzAJBgNVBAYTAklFMRAwDgYDVQQIDAdtdW5zdGVyMREwDwYDVQQHDAhsaW1l\r\n"  \
    "cmljazEMMAoGA1UECgwDYWRpMREwDwYDVQQLDAhzb2Z0d2FyZTEVMBMGA1UEAwwM\r\n"  \
    "MTkyLjE2OC4wLjI5MSUwIwYJKoZIhvcNAQkBFhZzYWRoYmgucnlhbkBhbmFsb2cu\r\n"  \
    "Y29tMB4XDTI2MDUwNjE0MDUyOVoXDTI3MDUwNjE0MDUyOVowgZExCzAJBgNVBAYT\r\n"  \
    "AklFMRAwDgYDVQQIDAdtdW5zdGVyMREwDwYDVQQHDAhsaW1lcmljazEMMAoGA1UE\r\n"  \
    "CgwDYWRpMREwDwYDVQQLDAhzb2Z0d2FyZTEVMBMGA1UEAwwMMTkyLjE2OC4wLjI5\r\n"  \
    "MSUwIwYJKoZIhvcNAQkBFhZzYWRoYmgucnlhbkBhbmFsb2cuY29tMFkwEwYHKoZI\r\n"  \
    "zj0CAQYIKoZIzj0DAQcDQgAEs+gwqDK18ikIDWRJoz9pNDZOtIwEH/HrP1XQYsyZ\r\n"  \
    "FZgB15F98CaIOc+xBGBsISayyi/PwwOmcpeIN0/4T9u5G6NCMEAwHQYDVR0OBBYE\r\n"  \
    "FI60u8+C7b2ACyPd8/OC51rdu/EyMB8GA1UdIwQYMBaAFO+VzGC7W1YDzCzXhVMD\r\n"  \
    "3I5Aw+LVMAoGCCqGSM49BAMCA0kAMEYCIQDHrzptTaP2ReKVE1I95IgrxeKOsrXs\r\n"  \
    "t8zO08VsAUEbGAIhAOfY3d5PtQMdmheWCyHgMA71RetmRnsM5Vww1H78fkSP\r\n"      \
    "-----END CERTIFICATE-----\r\n"

#define DEVICE_PRIVATE_KEY                                                  \
    "-----BEGIN EC PRIVATE KEY-----\r\n"                                    \
    "MHcCAQEEINIo3DeOxy48IWm/IXUt3E7Z92MdLSm5jbghjeFfEN9QoAoGCCqGSM49\r\n"  \
    "AwEHoUQDQgAEs+gwqDK18ikIDWRJoz9pNDZOtIwEH/HrP1XQYsyZFZgB15F98CaI\r\n"  \
    "Oc+xBGBsISayyi/PwwOmcpeIN0/4T9u5Gw==\r\n"                              \
    "-----END EC PRIVATE KEY-----\r\n"
#endif
