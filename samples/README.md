### Introduction

This sample provides sample application code that can be configured at build time
with the following parameters in *config/app_config.h*:

### Device Configuration Instructions

Depending on the authentication type of your device, the *config/app_config.h* 
needs to contain the information below:

##### Basic Device Parameters

* CPID: *Settings->Key Vault* in IoTConnect Web UI
* Environment: *Settings->Key Vault* in IoTConnect Web UI
* Unique Device ID (DUID): The device Unique ID in IoTConnect Web UI of your device.

##### Authentication types and relevant parameters depending on the type (IOTCONNECT_AUTH_TYPE):

* **Token (IOTC_AT_TOKEN):** This low security method requires no parameters. 
Any device with this authentication method can connect to IoTConnect given the above Basic Device Parameters.    
* **Symmetric Key (IOTC_AT_SYMMETRIC_KEY):** Symmetric Key authentication requires that *IOTCONNECT_SYMMETRIC_KEY* 
is set to either Primary or Secondary key.     
* **x509 (IOTC_AT_X509):** For devices with CA Certificate or Self-signed Certificate authentication types 
store the device certificate and private key at the certs directory as *client-crt.pem* and *client-kep.pem* respectively before building.
* **TPM (IOTC_AT_TPM):** If *IOTCONNECT_DUID* is blank, the TPM Registration ID will be obtained from TPM and it will be used in place of *IOTCONNECT_DUID*.
*IOTCONNECT_SCOPE_ID* must be set to the Scope ID provided in your *Settings->Key Vault* section under the *DPS* tab as *Scope ID for TPM Devices*.

See build instructions at thetop of theis repository on details on how to build the project.