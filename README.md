# aged
Age verifacation daemon for linux systems to comply with California's Digital Age Assurance Act cCalifornia's Digital Age Assurance Act.


[AVS Design Report.md](https://github.com/user-attachments/files/25797958/AVS.Design.Report.md)
# **Age Verification System (AVS) Design Report**

## **Executive Summary**

This report outlines the technical design for a privacy-focused Age Verification System (AVS) tailored for Linux distributions. The primary objective is to comply with California’s digital age assurance mandates (AB 1043\) by categorizing users into four specific age brackets while maintaining strict data minimization and local-only persistence.

## **1\. Compliance Framework: Age Brackets**

To ensure legal compliance while protecting user privacy, the system abstracts chronological age into four broad integer brackets. This prevents the storage of sensitive PII (Personally Identifiable Information) like exact birthdates.

| Bracket | Age Group | System Code | Description |
| :---- | :---- | :---- | :---- |
| **Child** | Under 13 | 0 | Restricted access; highest privacy protections. |
| **Young Teen** | 13 to 15 | 1 | Transitionary bracket; moderate restrictions. |
| **Old Teen** | 16 to 17 | 2 | Near-adult bracket; reduced restrictions. |
| **Adult** | 18 or older | 3 | Full access to all system features and content. |

## **2\. Technical Architecture**

The system utilizes a client-server model localized entirely to the host machine, leveraging Unix Domain Sockets for Inter-Process Communication (IPC).

### **2.1 Components**

* **Storage File (/etc/age\_bracket)**: A root-owned file containing the single-byte integer code representing the current user's bracket.  
* **System Daemon (aged)**: A multi-threaded C service running with root privileges. It acts as the exclusive gatekeeper for the storage file.  
* **Unix Domain Socket (/run/aged.sock)**: The communication interface. It is configured with permissions allowing unprivileged user applications to query (GET) the status.  
* **Client Library (libage)**: A wrapper library providing a simple API for developers to integrate age-checks into their software.

### **2.2 Communication Flow**

1. **Request**: A user application calls get\_age\_bracket() via libage.  
2. **IPC**: libage connects to /run/aged.sock and sends a GET command.  
3. **Processing**: The aged daemon receives the request, reads the local /etc/age\_bracket file, and identifies the integer code.  
4. **Response**: The daemon returns the integer code to the application; the application then gates content or features accordingly.

## **3\. Security & Privacy Principles**

### **3.1 Persistence Control**

By placing the age data in /etc/, the system leverages standard Linux filesystem hierarchy (FHS) security. Only a process with root-level capabilities (the daemon) can modify the age bracket, preventing users or malicious scripts from self-elevating their age status.

### **3.2 Data Minimization**

The system follows the "Privacy by Design" principle. No birthdates, names, or government IDs are stored within the OS. The only data point known to the system is a non-identifiable integer ranging from 0 to 3\.

### **3.3 Access Control**

The daemon strictly enforces a read-only policy for standard users. While any application can verify an age, only verified administrative processes (through the daemon) can trigger a change in the bracket, ensuring the integrity of the compliance signal.

## **4\. Conclusion**

The proposed AVS provides a robust, standardized method for Linux distributions to meet legal age-gating requirements. By keeping all verification logic and data local to the machine, the system avoids the privacy pitfalls of centralized identity providers while offering a seamless experience for both developers and users.
