/*
 Hospital Management System - Single File
 Corrected: public inheritance, ordering, and using namespace std
 Build: g++ -std=c++17 -O2 -o hospital HospitalManagement.cpp
 Run: ./hospital
*/

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <algorithm>
#include <iomanip>
#include <limits>

using namespace std;

// Forward declaration
class HospitalSystem;

// Utility input helpers
int readIntInRange(int minV, int maxV) {
    int x;
    while (true) {
        if (!(cin >> x)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Enter a number: ";
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        if (x < minV || x > maxV) {
            cout << "Enter a number between " << minV << " and " << maxV << ": ";
            continue;
        }
        return x;
    }
}

string readNonEmptyLine(const string &prompt = "") {
    string s;
    while (true) {
        if (!prompt.empty()) cout << prompt;
        getline(cin, s);
        if (s.empty()) {
            cout << "Input cannot be empty. Try again.\n";
            continue;
        }
        return s;
    }
}

string readLineAllowEmpty(const string &prompt = "") {
    string s;
    if (!prompt.empty()) cout << prompt;
    getline(cin, s);
    return s;
}

// Role enumeration
enum class Role { ADMIN, DOCTOR, NURSE, PHARMACIST, ACCOUNTS };

string roleToString(Role r) {
    switch (r) {
        case Role::ADMIN: return "Admin";
        case Role::DOCTOR: return "Doctor";
        case Role::NURSE: return "Nurse";
        case Role::PHARMACIST: return "Pharmacist";
        case Role::ACCOUNTS: return "Accounts Manager";
        default: return "Unknown";
    }
}

// Billing system
class Bill {
public:
    enum class Status { PENDING, PARTIALLY_PAID, FULLY_CLEARED };

    void addCharge(const string &desc, double amount) {
        if (amount <= 0) return;
        charges.push_back({desc, amount});
        updateStatus();
    }

    void addPayment(const string &method, double amount) {
        if (amount <= 0) return;
        payments.push_back({method, amount});
        updateStatus();
    }

    double totalCharges() const {
        double sum = 0;
        for (auto &c : charges) sum += c.second;
        return sum;
    }

    double totalPayments() const {
        double sum = 0;
        for (auto &p : payments) sum += p.second;
        return sum;
    }

    double balance() const {
        return totalCharges() - totalPayments();
    }

    Status getStatus() const { return status; }
    void setStatus(Status s) { status = s; }

    void printBillSummary() const {
        cout << "---- Bill Summary ----\n";
        cout << fixed << setprecision(2);
        cout << "Charges:\n";
        if (charges.empty()) cout << "  (none)\n";
        for (auto &c : charges) cout << "  " << c.first << " : $" << c.second << "\n";
        cout << "Payments:\n";
        if (payments.empty()) cout << "  (none)\n";
        for (auto &p : payments) cout << "  " << p.first << " : $" << p.second << "\n";
        cout << "Total Charges: $" << totalCharges() << "\n";
        cout << "Total Payments: $" << totalPayments() << "\n";
        cout << "Balance: $" << balance() << "\n";
        cout << "Status: " << statusToString(status) << "\n";
        cout << "----------------------\n";
    }

private:
    vector<pair<string,double>> charges;
    vector<pair<string,double>> payments;
    Status status = Status::PENDING;

    void updateStatus() {
        double bal = balance();
        if (bal <= 0.0) status = Status::FULLY_CLEARED;
        else if (totalPayments() > 0.0) status = Status::PARTIALLY_PAID;
        else status = Status::PENDING;
    }

    string statusToString(Status s) const {
        switch (s) {
            case Status::PENDING: return "Pending";
            case Status::PARTIALLY_PAID: return "Partially Paid";
            case Status::FULLY_CLEARED: return "Fully Cleared";
            default: return "Unknown";
        }
    }
};

// Patient record
class Patient {
public:
    Patient(int id_, const string &name_, int age_, const string &gender_,
            const string &symptoms_, const string &admissionDate_)
        : id(id_), name(name_), age(age_), gender(gender_),
          symptoms(symptoms_), admissionDate(admissionDate_) {}

    int getId() const { return id; }
    string getName() const { return name; }

    void addDiagnosis(const string &d) {
        if (!d.empty()) diagnoses.push_back(d);
    }

    void addMedicalNote(const string &note) {
        if (!note.empty()) medicalNotes.push_back(note);
    }

    void addPrescription(const string &presc) {
        if (!presc.empty()) prescriptions.push_back(presc);
    }

    Bill &getBill() { return bill; }
    const Bill &getBill() const { return bill; }

    void printBasicInfo() const {
        cout << "Patient ID: " << id << "\n";
        cout << "Name: " << name << ", Age: " << age << ", Gender: " << gender << "\n";
        cout << "Symptoms: " << symptoms << "\n";
        cout << "Date of admission: " << admissionDate << "\n";
    }

    void printFullRecord() const {
        printBasicInfo();
        cout << "Diagnoses:\n";
        if (diagnoses.empty()) cout << "  (none)\n";
        for (auto &d : diagnoses) cout << "  - " << d << "\n";
        cout << "Medical Notes:\n";
        if (medicalNotes.empty()) cout << "  (none)\n";
        for (auto &n : medicalNotes) cout << "  - " << n << "\n";
        cout << "Prescriptions:\n";
        if (prescriptions.empty()) cout << "  (none)\n";
        for (auto &p : prescriptions) cout << "  - " << p << "\n";
        bill.printBillSummary();
    }

private:
    int id;
    string name;
    int age;
    string gender;
    string symptoms;
    string admissionDate;

    vector<string> diagnoses;
    vector<string> medicalNotes;
    vector<string> prescriptions;
    Bill bill;
};

// Base User class
class User {
public:
    User(const string &username_, const string &password_, Role role_)
        : username(username_), password(password_), role(role_) {}
    virtual ~User() = default;

    string getUsername() const { return username; }
    Role getRole() const { return role; }

    bool checkPassword(const string &pw) const { return pw == password; }
    void setPassword(const string &pw) { password = pw; }

    // showMenu will be defined by derived classes (definitions later)
    virtual void showMenu(HospitalSystem &sys) = 0;

protected:
    string username;
    string password;
    Role role;
};

// Derived role classes - declarations (definitions for showMenu after HospitalSystem)
class AdminUser : public User {
public:
    AdminUser(const string &username_, const string &password_)
        : User(username_, password_, Role::ADMIN) {}
    void showMenu(HospitalSystem &sys) override;
};

class NurseUser : public User {
public:
    NurseUser(const string &username_, const string &password_)
        : User(username_, password_, Role::NURSE) {}
    void showMenu(HospitalSystem &sys) override;
};

class DoctorUser : public User {
public:
    DoctorUser(const string &username_, const string &password_)
        : User(username_, password_, Role::DOCTOR) {}
    void showMenu(HospitalSystem &sys) override;
};

class PharmacistUser : public User {
public:
    PharmacistUser(const string &username_, const string &password_)
        : User(username_, password_, Role::PHARMACIST) {}
    void showMenu(HospitalSystem &sys) override;
};

class AccountsUser : public User {
public:
    AccountsUser(const string &username_, const string &password_)
        : User(username_, password_, Role::ACCOUNTS) {}
    void showMenu(HospitalSystem &sys) override;
};

// HospitalSystem coordinates everything
class HospitalSystem {
public:
    HospitalSystem() {
        // Create default admin
        users.push_back(make_shared<AdminUser>("admin", "admin123"));
    }

    void run();

    // User management
    bool usernameExists(const string &uname) const {
        for (auto &u : users) if (u->getUsername() == uname) return true;
        return false;
    }

    void addUser(shared_ptr<User> user) {
        users.push_back(move(user));
    }

    bool deleteUser(const string &username) {
        auto it = find_if(users.begin(), users.end(),
            [&](const shared_ptr<User> &u) { return u->getUsername() == username; });
        if (it == users.end()) return false;
        // Prevent deleting the last admin
        if ((*it)->getRole() == Role::ADMIN) {
            int adminCount = 0;
            for (auto &u : users) if (u->getRole() == Role::ADMIN) adminCount++;
            if (adminCount <= 1) {
                cout << "Cannot delete the last Admin account.\n";
                return false;
            }
        }
        users.erase(it);
        return true;
    }

    void listEmployees() const {
        cout << "---- Registered Employees ----\n";
        for (auto &u : users) {
            cout << "Username: " << u->getUsername() << " | Role: " << roleToString(u->getRole()) << "\n";
        }
        cout << "------------------------------\n";
    }

    shared_ptr<User> authenticate(const string &username, const string &password) {
        for (auto &u : users) {
            if (u->getUsername() == username && u->checkPassword(password)) return u;
        }
        return nullptr;
    }

    // Patient management
    int registerPatient(const string &name, int age, const string &gender,
                        const string &symptoms, const string &date) {
        int id = ++lastPatientId;
        patients.emplace_back(id, name, age, gender, symptoms, date);
        cout << "Patient registered with ID: " << id << "\n";
        return id;
    }

    Patient* findPatientById(int id) {
        for (auto &p : patients) if (p.getId() == id) return &p;
        return nullptr;
    }

    void listPatientsBrief() const {
        cout << "---- Patients (brief) ----\n";
        for (auto &p : patients) {
            cout << "ID: " << p.getId() << " | Name: " << p.getName() << "\n";
        }
        cout << "--------------------------\n";
    }

    const vector<shared_ptr<User>>& getUsers() const { return users; }

private:
    vector<shared_ptr<User>> users;
    vector<Patient> patients;
    int lastPatientId = 0;
};

// Definitions of showMenu functions for each role (after HospitalSystem defined)

// AdminUser
void AdminUser::showMenu(HospitalSystem &sys) {
    while (true) {
        cout << "\n--- Admin Menu ---\n";
        cout << "1. Register employee\n";
        cout << "2. Delete employee\n";
        cout << "3. View all employees\n";
        cout << "4. Change my password\n";
        cout << "5. Logout (Back)\n";
        cout << "Choose an option: ";
        int opt = readIntInRange(1,5);
        if (opt == 1) {
            string uname = readNonEmptyLine("Enter username for employee: ");
            if (sys.usernameExists(uname)) {
                cout << "Username already exists.\n";
                continue;
            }
            cout << "Select role:\n";
            cout << "1. Doctor\n2. Nurse\n3. Pharmacist\n4. Accounts Manager\nChoose role: ";
            int r = readIntInRange(1, 4);
            string pw = readNonEmptyLine("Set password for employee: ");
            shared_ptr<User> newUser;
            switch (r) {
                case 1: newUser = make_shared<DoctorUser>(uname, pw); break;
                case 2: newUser = make_shared<NurseUser>(uname, pw); break;
                case 3: newUser = make_shared<PharmacistUser>(uname, pw); break;
                case 4: newUser = make_shared<AccountsUser>(uname, pw); break;
                default: break;
            }
            sys.addUser(newUser);
            cout << "Employee registered: " << uname << " (" << roleToString(newUser->getRole()) << ")\n";
        } else if (opt == 2) {
            sys.listEmployees();
            string del = readNonEmptyLine("Enter username to delete (or type 'back' to cancel): ");
            if (del == "back") continue;
            if (!sys.usernameExists(del)) {
                cout << "No such user.\n";
                continue;
            }
            if (del == username) {
                cout << "You cannot delete your own account here.\n";
                continue;
            }
            if (sys.deleteUser(del)) cout << "Deleted user: " << del << "\n";
            else cout << "Failed to delete user.\n";
        } else if (opt == 3) {
            sys.listEmployees();
        } else if (opt == 4) {
            string newpw = readNonEmptyLine("Enter new password: ");
            setPassword(newpw);
            cout << "Password updated.\n";
        } else {
            break;
        }
    }
}

// NurseUser
void NurseUser::showMenu(HospitalSystem &sys) {
    while (true) {
        cout << "\n--- Nurse Menu ---\n";
        cout << "1. Register new patient\n";
        cout << "2. View basic patient information\n";
        cout << "3. Change my password\n";
        cout << "4. Logout (Back)\n";
        cout << "Choose an option: ";
        int opt = readIntInRange(1,4);
        if (opt == 1) {
            string name = readNonEmptyLine("Full name: ");
            int age;
            while (true) {
                cout << "Age: ";
                if ((cin >> age) && age > 0) {
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid age.\n";
            }
            string gender = readNonEmptyLine("Gender: ");
            string symptoms = readNonEmptyLine("Symptoms: ");
            string date = readNonEmptyLine("Date of admission (YYYY-MM-DD): ");
            sys.registerPatient(name, age, gender, symptoms, date);
        } else if (opt == 2) {
            sys.listPatientsBrief();
            cout << "Enter patient ID to view (0 to cancel): ";
            int id = readIntInRange(0, 1000000);
            if (id == 0) continue;
            Patient* p = sys.findPatientById(id);
            if (!p) { cout << "Patient not found.\n"; continue; }
            p->printBasicInfo();
        } else if (opt == 3) {
            string newpw = readNonEmptyLine("Enter new password: ");
            setPassword(newpw);
            cout << "Password updated.\n";
        } else break;
    }
}

// DoctorUser
void DoctorUser::showMenu(HospitalSystem &sys) {
    while (true) {
        cout << "\n--- Doctor Menu ---\n";
        cout << "1. View registered patient records (brief)\n";
        cout << "2. View full patient record by ID\n";
        cout << "3. Add diagnostic information\n";
        cout << "4. Add medical notes\n";
        cout << "5. Prescribe medication\n";
        cout << "6. Add billing entry (consultation/tests)\n";
        cout << "7. Change my password\n";
        cout << "8. Logout (Back)\n";
        cout << "Choose an option: ";
        int opt = readIntInRange(1,8);
        if (opt == 1) {
            sys.listPatientsBrief();
        } else if (opt == 2) {
            cout << "Enter patient ID (0 to cancel): ";
            int id = readIntInRange(0, 1000000);
            if (id == 0) continue;
            Patient* p = sys.findPatientById(id);
            if (!p) { cout << "Patient not found.\n"; continue; }
            p->printFullRecord();
        } else if (opt == 3) {
            cout << "Enter patient ID: ";
            int id = readIntInRange(1, 1000000);
            Patient* p = sys.findPatientById(id);
            if (!p) { cout << "Patient not found.\n"; continue; }
            string diag = readNonEmptyLine("Enter diagnostic information: ");
            p->addDiagnosis(diag);
            cout << "Diagnosis added.\n";
        } else if (opt == 4) {
            cout << "Enter patient ID: ";
            int id = readIntInRange(1, 1000000);
            Patient* p = sys.findPatientById(id);
            if (!p) { cout << "Patient not found.\n"; continue; }
            string note = readNonEmptyLine("Enter medical note: ");
            p->addMedicalNote(note);
            cout << "Medical note added.\n";
        } else if (opt == 5) {
            cout << "Enter patient ID: ";
            int id = readIntInRange(1, 1000000);
            Patient* p = sys.findPatientById(id);
            if (!p) { cout << "Patient not found.\n"; continue; }
            string presc = readNonEmptyLine("Enter prescription details: ");
            p->addPrescription(presc);
            cout << "Prescription recorded.\n";
        } else if (opt == 6) {
            cout << "Enter patient ID: ";
            int id = readIntInRange(1, 1000000);
            Patient* p = sys.findPatientById(id);
            if (!p) { cout << "Patient not found.\n"; continue; }
            string desc = readNonEmptyLine("Charge description (e.g., Consultation, X-ray): ");
            double amt;
            while (true) {
                cout << "Amount: $";
                if ((cin >> amt) && amt > 0.0) { cin.ignore(numeric_limits<streamsize>::max(), '\n'); break; }
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid amount.\n";
            }
            p->getBill().addCharge(desc, amt);
            cout << "Charge added to bill.\n";
        } else if (opt == 7) {
            string newpw = readNonEmptyLine("Enter new password: ");
            setPassword(newpw);
            cout << "Password updated.\n";
        } else break;
    }
}

// PharmacistUser
void PharmacistUser::showMenu(HospitalSystem &sys) {
    while (true) {
        cout << "\n--- Pharmacist Menu ---\n";
        cout << "1. View patient medical record (full)\n";
        cout << "2. Record medication dispensed\n";
        cout << "3. Add medication cost to patient bill\n";
        cout << "4. Change my password\n";
        cout << "5. Logout (Back)\n";
        cout << "Choose an option: ";
        int opt = readIntInRange(1,5);
        if (opt == 1) {
            cout << "Enter patient ID: ";
            int id = readIntInRange(1, 1000000);
            Patient* p = sys.findPatientById(id);
            if (!p) { cout << "Patient not found.\n"; continue; }
            p->printFullRecord();
        } else if (opt == 2) {
            cout << "Enter patient ID: ";
            int id = readIntInRange(1, 1000000);
            Patient* p = sys.findPatientById(id);
            if (!p) { cout << "Patient not found.\n"; continue; }
            string med = readNonEmptyLine("Enter medication details dispensed: ");
            p->addPrescription(med); // store medications as prescriptions for now
            cout << "Medication dispensed and recorded.\n";
        } else if (opt == 3) {
            cout << "Enter patient ID: ";
            int id = readIntInRange(1, 1000000);
            Patient* p = sys.findPatientById(id);
            if (!p) { cout << "Patient not found.\n"; continue; }
            string desc = readNonEmptyLine("Medication description: ");
            double amt;
            while (true) {
                cout << "Amount: $";
                if ((cin >> amt) && amt > 0.0) { cin.ignore(numeric_limits<streamsize>::max(), '\n'); break; }
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid amount.\n";
            }
            p->getBill().addCharge(desc, amt);
            cout << "Medication cost added to bill.\n";
        } else if (opt == 4) {
            string newpw = readNonEmptyLine("Enter new password: ");
            setPassword(newpw);
            cout << "Password updated.\n";
        } else break;
    }
}

// AccountsUser
void AccountsUser::showMenu(HospitalSystem &sys) {
    while (true) {
        cout << "\n--- Accounts Manager Menu ---\n";
        cout << "1. View complete patient bill\n";
        cout << "2. Record payment made\n";
        cout << "3. Mark bill status manually\n";
        cout << "4. Change my password\n";
        cout << "5. Logout (Back)\n";
        cout << "Choose an option: ";
        int opt = readIntInRange(1,5);
        if (opt == 1) {
            cout << "Enter patient ID: ";
            int id = readIntInRange(1, 1000000);
            Patient* p = sys.findPatientById(id);
            if (!p) { cout << "Patient not found.\n"; continue; }
            p->getBill().printBillSummary();
        } else if (opt == 2) {
            cout << "Enter patient ID: ";
            int id = readIntInRange(1, 1000000);
            Patient* p = sys.findPatientById(id);
            if (!p) { cout << "Patient not found.\n"; continue; }
            string method = readNonEmptyLine("Payment method (e.g., Cash/Card/Insurance): ");
            double amt;
            while (true) {
                cout << "Amount paid: $";
                if ((cin >> amt) && amt > 0.0) { cin.ignore(numeric_limits<streamsize>::max(), '\n'); break; }
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid amount.\n";
            }
            p->getBill().addPayment(method, amt);
            cout << "Payment recorded.\n";
        } else if (opt == 3) {
            cout << "Enter patient ID: ";
            int id = readIntInRange(1, 1000000);
            Patient* p = sys.findPatientById(id);
            if (!p) { cout << "Patient not found.\n"; continue; }
            cout << "Select status:\n1. Fully cleared\n2. Partially paid\n3. Pending\nChoose: ";
            int s = readIntInRange(1,3);
            Bill::Status ns;
            switch (s) {
                case 1: ns = Bill::Status::FULLY_CLEARED; break;
                case 2: ns = Bill::Status::PARTIALLY_PAID; break;
                default: ns = Bill::Status::PENDING; break;
            }
            p->getBill().setStatus(ns);
            cout << "Bill status updated.\n";
        } else if (opt == 4) {
            string newpw = readNonEmptyLine("Enter new password: ");
            setPassword(newpw);
            cout << "Password updated.\n";
        } else break;
    }
}

// HospitalSystem::run implementation
void HospitalSystem::run() {
    while (true) {
        cout << "\n=== Hospital Management System ===\n";
        cout << "1. Login\n";
        cout << "2. Exit\n";
        cout << "Choose an option: ";
        int opt = readIntInRange(1,2);
        if (opt == 2) {
            cout << "Exiting. Goodbye.\n";
            break;
        }
        // Login
        string uname = readNonEmptyLine("Username: ");
        string pw = readNonEmptyLine("Password: ");
        auto user = authenticate(uname, pw);
        if (!user) {
            cout << "Invalid username or password.\n";
            continue;
        }
        cout << "Login successful. Welcome, " << user->getUsername() << " (" << roleToString(user->getRole()) << ")\n";
        user->showMenu(*this);
        cout << "Logged out.\n";
    }
}

// Main
int main() {
    HospitalSystem hs;
    cout << "Default admin account created: username='admin', password='admin123'\n";
    hs.run();
    return 0;
}