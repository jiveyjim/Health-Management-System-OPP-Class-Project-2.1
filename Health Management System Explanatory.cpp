/*
    Single-file console application (HospitalManagement.cpp).
    Main components:
        Utility input helpers (robust user input, validation).
        Domain classes: Bill, Patient.
        User hierarchy: User (base) and derived roles (AdminUser, NurseUser, DoctorUser, PharmacistUser, AccountsUser).
        Controller: HospitalSystem — stores users and patients, coordinates flows, authentication, registration, and search.
        Role menus implemented via virtual method showMenu(HospitalSystem&), using polymorphism so each user sees their own menu.
    Uses STL: vector, string, shared_ptr, pair, algorithms, i/o manipulators.
    Role-based access control implemented by only allowing actions through the methods exposed by each role’s showMenu.

Now the detailed walkthrough.

Top-of-file includes and using directive

    #include <...>
        Brings in the C++ Standard Library facilities used: iostream (console IO), string, vector (container), memory (shared_ptr), map (unused but included originally), algorithm (find_if, etc.), iomanip (formatted IO), limits (numeric limits).
    using namespace std;
        Convenience so we don't prefix std:: everywhere. (OK for small projects; avoid in headers or large codebases.)

Forward declaration

    class HospitalSystem;
        Declares HospitalSystem exists so User::showMenu and derived class declarations may reference HospitalSystem before HospitalSystem is defined. This prevents circular dependency issues.

Utility input helpers

    int readIntInRange(int minV, int maxV)
        Reads an integer from cin, validates it is an integer, clears cin on failure, ignores the rest of the line, and enforces min/max range. Loops until a valid value is entered.
        Purpose: centralize integer input validation and repetition prevention.

    string readNonEmptyLine(const string &prompt = "")
        Repeatedly shows prompt, reads a whole line (getline) and rejects empty input.
        Purpose: force non-empty string fields (e.g., username, password, patient name).

    string readLineAllowEmpty(const string &prompt = "")
        Gets a whole line without forcing non-empty.

Why these helpers matter

    They centralize input validation and keep menu code readable. They also avoid a common pitfall where mixing operator>> and getline breaks input flow.

Role enumeration and roleToString

    enum class Role { ADMIN, DOCTOR, NURSE, PHARMACIST, ACCOUNTS };
        Strongly-typed enumeration that represents user roles.
    string roleToString(Role r)
        Convenience to convert Role values to human-readable names for menus and lists.

Bill class (manages billing)

    class Bill { ... private: vector<pair<string,double>> charges; vector<pair<string,double>> payments; Status status; ... }
        Encapsulates billing data: list of (description, amount) charges and payments.
        Status nested enum: PENDING, PARTIALLY_PAID, FULLY_CLEARED.

    Public methods:
        void addCharge(desc, amount)
            Adds a charge (only positive amounts) and calls updateStatus().
        void addPayment(method, amount)
            Adds a payment and calls updateStatus().
        double totalCharges(), totalPayments(), balance()
            Compute totals on-the-fly using STL iteration. No persisted "cached" totals — simpler and less error-prone.
        Status getStatus(), void setStatus(Status s)
            Accessor and manual mutator used by Accounts role to override billing state if needed.
        void printBillSummary()
            Nicely formatted printing of charges/payments/totals/status.

    Private helper updateStatus()
        Recomputes status according to balance and payments.
        Keeps internal invariants: if balance <= 0 => FULLY_CLEARED, else if any payment => PARTIALLY_PAID, else PENDING.

OOP mapping: Bill demonstrates encapsulation (all data is private), single responsibility (only billing logic), and using STL to store collections.

Patient class (patient record)

    Members:
        int id;
        name, age, gender, symptoms, admissionDate
        vectors: diagnoses, medicalNotes, prescriptions
        Bill bill; (each patient has its own Bill)
    Methods:
        Accessors: getId(), getName()
        Mutators: addDiagnosis(), addMedicalNote(), addPrescription()
        Bill &getBill()
        printBasicInfo(), printFullRecord()
    Responsibilities:
        Holds all patient-related data and provides methods to mutate it. printFullRecord prints everything and calls bill.printBillSummary().

OOP mapping: Patient encapsulates patient data and acts as a composite (it contains a Bill object). Methods present an abstracted interface so other classes interact with patients without seeing internal implementation.

User base class (abstraction for roles)

    class User { public: User(const string &username_, const string &password_, Role role_); virtual ~User() = default; string getUsername() const; Role getRole() const; bool checkPassword(const string &pw) const; void setPassword(const string &pw); virtual void showMenu(HospitalSystem &sys) = 0; // pure virtual protected: string username; string password; Role role; };

    Purpose:
        Abstract class representing a user account in the system.
        Encapsulates credentials and role.
        The pure virtual method showMenu(HospitalSystem&) defines the contract every role must fulfill: a menu with role-specific actions.
        Because it’s polymorphic (virtual destructor and virtual functions), you can store shared_ptr<User> referencing derived objects and call showMenu on them — runtime dispatch picks the correct derived implementation.

OOP mapping: This is abstraction (User defines what a user is and what they must do), encapsulation (private/protected data), and polymorphism (derived types implement showMenu).

Derived user classes (AdminUser, NurseUser, DoctorUser, PharmacistUser, AccountsUser)

    Each derived class inherits publicly from User: class NurseUser : public User { ... }
        public inheritance establishes an "is-a" relationship: a NurseUser is a User.
        Each provides an implementation of showMenu(HospitalSystem &sys).
    Each showMenu:
        Presents a loop with options for allowed operations.
        Uses readIntInRange and readNonEmptyLine to get validated input.
        Calls HospitalSystem methods to perform actions (register patients, modify patients, add billing entries, etc.).
        Includes "Change my password" option calling setPassword(newpw).
        Includes a "Logout (Back)" option that breaks the loop and returns to caller (HospitalSystem::run).

Role-specific capabilities (implemented in showMenu):

    AdminUser:
        Create employees (Doctor, Nurse, Pharmacist, AccountsManager): creates the appropriate derived shared_ptr and adds it to HospitalSystem users vector.
        Delete employee (with guard: cannot delete the last admin).
        View employees list (sys.listEmployees()).
        Change own password.
    NurseUser:
        Register new patient (calls sys.registerPatient).
        View basic patient info by ID.
    DoctorUser:
        View patient records, add diagnosis, add medical notes, prescribe medication (adds to prescriptions vector), add billing entries (consultation/tests), change own password.
    PharmacistUser:
        View full records, record medication dispensed (stored as a prescription entry), add medication cost to patient bill.
    AccountsUser:
        View complete patient bills, record payments (bill.addPayment), mark bill status manually, change own password.

OOP mapping: Derived classes demonstrate inheritance and polymorphism (call showMenu on User pointer); each role encapsulates its permitted actions, enforcing role-based access control because only the derived showMenu exposes specific operations to users.

HospitalSystem class (controller, data store)

    Members:
        vector<shared_ptr<User>> users;
            Stores all user accounts polymorphically using shared_ptr<User>. Using shared_ptr allows copying these pointers into different places safely and simplifies lifetime management.
        vector<Patient> patients;
            In-memory patient list (stored by value).
        int lastPatientId = 0; (used to assign incremental patient IDs)
    Constructor:
        users.push_back(make_shared<AdminUser>("admin", "admin123"));
            Creates default admin account at startup.
    Methods:
        run()
            Top-level program loop: shows startup menu (Login / Exit), accepts credentials, authenticates, and on success calls user->showMenu(*this) to enter role-specific loop.
        usernameExists(const string&): checks whether username already exists (scan users vector).
        addUser(shared_ptr<User> user): adds a new user to users (move semantics used).
        deleteUser(username): finds and removes user; prevents deletion of the last admin.
        listEmployees(): prints all username + role pairs.
        authenticate(username, password): scans users, returns shared_ptr<User> if credentials match; otherwise nullptr.
        registerPatient(name, age, gender, symptoms, date): increments lastPatientId and adds a new Patient to patients vector.
        findPatientById(id): returns pointer to Patient in vector or nullptr if not found.
        listPatientsBrief(): prints patient id & name for selection.

Why shared_ptr<User>

    You store polymorphic objects (AdminUser, DoctorUser, ... ) in a single container typed as shared_ptr<User>. This requires public inheritance and a complete definition of User at the point of instantiation. shared_ptr ensures automatic memory cleanup of the heap-allocated derived objects.

How authentication and role-based dispatch works

    On login:
        HospitalSystem::run asks for username & password.
        authenticate returns shared_ptr<User> which points to the concrete derived instance.
        run then calls user->showMenu(*this).
        This is runtime polymorphism: the virtual function table routes the call to the derived class implementation, so the correct menu for that role is shown.
    Role-based access control:
        The only code that performs actions (e.g., adding charges, registering patients) is inside the showMenu implementations of each role. Because a non-admin user has no access to Admin's showMenu code, they cannot perform admin-only tasks unless you change the code.
        HospitalSystem provides actions that are called from showMenu — the role decides which of those actions to call.

Menu-driven interface and Input validation

    All menus are loops that present choices, read validated input using readIntInRange, and either perform actions or break to return to previous menu.
    Every menu includes a "Logout (Back)" option that returns control to caller; program termination occurs only when user chooses Exit at the top-level menu.
    All numeric inputs and required strings are validated to avoid invalid input states.

STL usage

    vector<T> for dynamic collections (users, patients, charges).
    shared_ptr<User> to store polymorphic user objects.
    pair<string,double> to store description/amount pairs.
    find_if and algorithm usage for deleting users by username.
    iomanip for formatting money output (fixed, setprecision).

Memory management

    Users are stored as shared_ptr<User> to heap-allocated derived objects created via make_shared<Derived>.
    Patients are stored by value in the vector<Patient>. That is simpler and acceptable because Patient is a reasonably small value-type here. findPatientById returns a pointer to an element inside that vector (address remains valid until the vector resizes).
    Risk: if vector<Patient> were to reallocate, pointers returned earlier would be invalidated; however, in the current program flow pointers are only used immediately after retrieval. For long-lived references or external references, consider storing patients with shared_ptr or using stable containers (list, deque) or using indices instead of raw pointers.

Key error you had earlier and how the corrected program addresses it

    The compiler errors you saw (no matching function for push_back with shared_ptr<Derived>) happen when Derived does not publicly inherit from Base or when the Base type is not a complete type at the point of construction.
    In this version:
        All derived classes inherit publicly: class DoctorUser : public User.
        User is fully defined before objects are constructed, so shared_ptr<Derived> converts to shared_ptr<User> correctly.

Flow example (typical runtime session)

    Program starts, default admin created and printed to console.
    HospitalSystem::run shows startup menu.
    User chooses Login, enters "admin"/"admin123".
    authenticate returns AdminUser instance pointer; user->showMenu(*this) calls AdminUser::showMenu.
    Admin registers a Nurse: AdminUser::showMenu calls make_shared<NurseUser>(uname, pw) and sys.addUser(newUser).
    Nurse logs in: NurseUser::showMenu can register patients (sys.registerPatient(...)), which appends a Patient to patients vector and prints ID.
    Doctor logs in and uses patient ID to add diagnosis and add billing charges (p->getBill().addCharge(desc, amt)).
    Pharmacist may add medication cost (p->getBill().addCharge) and add prescriptions.
    Accounts logs in to view bills and record payments (p->getBill().addPayment) and mark status manually (p->getBill().setStatus).
    Users can change their password (calls setPassword) which updates the stored password.

Mapping to the required OOP concepts

    Classes and objects: The program defines many classes (User, AdminUser, NurseUser, DoctorUser, PharmacistUser, AccountsUser, Patient, Bill, HospitalSystem) and instantiates objects of them during runtime.
    Encapsulation: Data members are private (or protected for User) and accessed only through public methods (getters/setters), preventing arbitrary external mutation.
    Abstraction: User provides an abstract interface (showMenu) while concrete roles implement behaviour. Patient and Bill hide internal structure behind methods.
    Inheritance: Role classes derive from User. This lets them reuse code (username, password fields and methods) and specialize behaviour (showMenu).
    Polymorphism: showMenu is virtual; HospitalSystem interacts with users through shared_ptr<User> and calls showMenu without knowing the concrete derived type at compile-time.
    Single Responsibility: Each class has a narrow responsibility: Bill handles billing, Patient holds patient data, User holds account info, HospitalSystem coordinates the application.

Important implementation details and gotchas

    Mixing operator>> and getline:
        The code carefully calls cin.ignore(...) after operator>> to remove the newline left in input buffer before calling getline. This avoids skipping input.
    Input range checking:
        readIntInRange enforces min and max; menus call it to avoid invalid options.
    Finding users/patients:
        Users are found by linear scan using find_if — acceptable for small datasets. For very large datasets, use unordered_map<string, shared_ptr<User>> keyed by username for O(1) lookup.
    Patient pointers:
        findPatientById returns a raw pointer to an element in the patients vector. That is OK as long as you don’t hold that pointer across vector modifications that might reallocate. For safer long-lived references, use shared_ptr<Patient> stored in vector<shared_ptr<Patient>>.
    Password storage:
        Passwords are stored in plain strings. For production, use hashing and secure input handling (no echo, salted hashes).
    Persistence:
        Current implementation stores everything in memory. If program exits, data is lost. To persist, implement serialization (save/load) to files.

How to read and follow the code in the file

    Top (includes + helpers): understand input helper patterns first.
    Domain classes (Bill, Patient): read next — these are the data models used across the app.
    User base and derived class declarations: gives you the shape of the role hierarchy.
    HospitalSystem class: central orchestrator (read this next) — it shows how users and patients are stored and manipulated.
    showMenu() definitions (role implementations): read these to see actual allowed operations per role, and how they call HospitalSystem methods or modify Patient/Bill.
    main(): simply constructs HospitalSystem and calls run().
 */
/*
 Single-file Hospital Management System with line-by-line comments.
 Build: g++ -std=c++17 -O2 -o hospital HospitalManagement_annotated.cpp
 Run: ./hospital
*/ // file header explaining purpose

#include <iostream> // for std::cin, std::cout, std::endl
#include <string> // for std::string
#include <vector> // for std::vector
#include <memory> // for std::shared_ptr, std::make_shared
#include <map> // included but not used heavily; available if needed
#include <algorithm> // for std::find_if
#include <iomanip> // for std::fixed and std::setprecision
#include <limits> // for std::numeric_limits

using namespace std; // convenience for this small single-file program

// Forward declaration of HospitalSystem so User-related declarations can reference it.
class HospitalSystem; // forward declare controller class to avoid circular dependency

// Utility: read an integer and enforce a range; loops until input is valid.
int readIntInRange(int minV, int maxV) { // function reads an int constrained to [minV,maxV]
    int x; // local variable to hold input
    while (true) { // loop until valid input provided
        if (!(cin >> x)) { // attempt to read integer; if fails...
            cin.clear(); // clear error flags on cin
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // discard rest of line
            cout << "Invalid input. Enter a number: "; // prompt user again
            continue; // restart loop
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // remove trailing newline after number
        if (x < minV || x > maxV) { // check range
            cout << "Enter a number between " << minV << " and " << maxV << ": "; // inform user
            continue; // restart loop
        }
        return x; // valid number, return it
    }
} // end readIntInRange

// Utility: read a non-empty line from stdin with an optional prompt.
string readNonEmptyLine(const string &prompt = "") { // reads a whole line and forces non-empty
    string s; // buffer for the line
    while (true) { // loop until non-empty line provided
        if (!prompt.empty()) cout << prompt; // show prompt if provided
        getline(cin, s); // read entire line
        if (s.empty()) { // reject empty input
            cout << "Input cannot be empty. Try again.\n"; // tell user
            continue; // loop again
        }
        return s; // return valid string
    }
} // end readNonEmptyLine

// Utility: read a line that may be empty, with optional prompt.
string readLineAllowEmpty(const string &prompt = "") { // simple wrapper around getline
    string s; // buffer
    if (!prompt.empty()) cout << prompt; // show prompt if provided
    getline(cin, s); // read line (may be empty)
    return s; // return the string
} // end readLineAllowEmpty

// Roles enumeration to represent user roles across system.
enum class Role { ADMIN, DOCTOR, NURSE, PHARMACIST, ACCOUNTS }; // strongly-typed enum for roles

// Convert Role enum to a human-readable string for menus and lists.
string roleToString(Role r) { // maps Role to string
    switch (r) { // switch on enum
        case Role::ADMIN: return "Admin"; // admin label
        case Role::DOCTOR: return "Doctor"; // doctor label
        case Role::NURSE: return "Nurse"; // nurse label
        case Role::PHARMACIST: return "Pharmacist"; // pharmacist label
        case Role::ACCOUNTS: return "Accounts Manager"; // accounts manager label
        default: return "Unknown"; // fallback
    }
} // end roleToString

// Bill class manages charges, payments, and status for a patient.
class Bill { // encapsulates billing operations and state
public: // public API for Bill
    enum class Status { PENDING, PARTIALLY_PAID, FULLY_CLEARED }; // possible bill states

    void addCharge(const string &desc, double amount) { // add a charge line item
        if (amount <= 0) return; // ignore non-positive amounts
        charges.push_back({desc, amount}); // append description+amount pair
        updateStatus(); // recalc status after change
    } // end addCharge

    void addPayment(const string &method, double amount) { // record a payment
        if (amount <= 0) return; // ignore invalid payments
        payments.push_back({method, amount}); // append payment record
        updateStatus(); // recalc status after change
    } // end addPayment

    double totalCharges() const { // compute sum of all charges
        double sum = 0; // accumulator
        for (auto &c : charges) sum += c.second; // add each amount
        return sum; // return total
    } // end totalCharges

    double totalPayments() const { // compute sum of all payments
        double sum = 0; // accumulator
        for (auto &p : payments) sum += p.second; // add each payment amount
        return sum; // return total
    } // end totalPayments

    double balance() const { // compute outstanding balance
        return totalCharges() - totalPayments(); // charges minus payments
    } // end balance

    Status getStatus() const { return status; } // accessor for status
    void setStatus(Status s) { status = s; } // manual override/set of status

    void printBillSummary() const { // pretty-print the bill summary to cout
        cout << "---- Bill Summary ----\n"; // header
        cout << fixed << setprecision(2); // format monetary values
        cout << "Charges:\n"; // charges header
        if (charges.empty()) cout << "  (none)\n"; // show none if empty
        for (auto &c : charges) cout << "  " << c.first << " : $" << c.second << "\n"; // print each charge
        cout << "Payments:\n"; // payments header
        if (payments.empty()) cout << "  (none)\n"; // show none if empty
        for (auto &p : payments) cout << "  " << p.first << " : $" << p.second << "\n"; // print each payment
        cout << "Total Charges: $" << totalCharges() << "\n"; // show totals
        cout << "Total Payments: $" << totalPayments() << "\n"; // show totals
        cout << "Balance: $" << balance() << "\n"; // show outstanding balance
        cout << "Status: " << statusToString(status) << "\n"; // show human-readable status
        cout << "----------------------\n"; // footer
    } // end printBillSummary

private: // internal state and helper
    vector<pair<string,double>> charges; // list of (description,amount) charges
    vector<pair<string,double>> payments; // list of (method,amount) payments
    Status status = Status::PENDING; // initial status default to pending

    void updateStatus() { // determine status from sums
        double bal = balance(); // compute current balance
        if (bal <= 0.0) status = Status::FULLY_CLEARED; // cleared if no balance
        else if (totalPayments() > 0.0) status = Status::PARTIALLY_PAID; // partial if any payment
        else status = Status::PENDING; // otherwise pending
    } // end updateStatus

    string statusToString(Status s) const { // helper to convert status to text
        switch (s) { // map enum to string
            case Status::PENDING: return "Pending"; // pending
            case Status::PARTIALLY_PAID: return "Partially Paid"; // partially paid
            case Status::FULLY_CLEARED: return "Fully Cleared"; // fully cleared
            default: return "Unknown"; // fallback
        }
    } // end statusToString
}; // end class Bill

// Patient class holds patient details, medical notes, prescriptions, and a Bill.
class Patient { // encapsulates patient-related data and methods
public: // public API for Patient
    Patient(int id_, const string &name_, int age_, const string &gender_,
            const string &symptoms_, const string &admissionDate_) // constructor with initialization list
        : id(id_), name(name_), age(age_), gender(gender_),
          symptoms(symptoms_), admissionDate(admissionDate_) {} // initialize fields

    int getId() const { return id; } // accessor for patient ID
    string getName() const { return name; } // accessor for name

    void addDiagnosis(const string &d) { if (!d.empty()) diagnoses.push_back(d); } // add diagnosis text
    void addMedicalNote(const string &note) { if (!note.empty()) medicalNotes.push_back(note); } // add note
    void addPrescription(const string &presc) { if (!presc.empty()) prescriptions.push_back(presc); } // add prescription

    Bill &getBill() { return bill; } // non-const access to bill for modification
    const Bill &getBill() const { return bill; } // const access to bill for reading

    void printBasicInfo() const { // print core patient details
        cout << "Patient ID: " << id << "\n"; // print id
        cout << "Name: " << name << ", Age: " << age << ", Gender: " << gender << "\n"; // print demographics
        cout << "Symptoms: " << symptoms << "\n"; // print symptoms
        cout << "Date of admission: " << admissionDate << "\n"; // print admission date
    } // end printBasicInfo

    void printFullRecord() const { // print entire patient record including bill
        printBasicInfo(); // start with basic info
        cout << "Diagnoses:\n"; // diagnoses header
        if (diagnoses.empty()) cout << "  (none)\n"; // none message
        for (auto &d : diagnoses) cout << "  - " << d << "\n"; // print each diagnosis
        cout << "Medical Notes:\n"; // notes header
        if (medicalNotes.empty()) cout << "  (none)\n"; // none message
        for (auto &n : medicalNotes) cout << "  - " << n << "\n"; // print notes
        cout << "Prescriptions:\n"; // prescriptions header
        if (prescriptions.empty()) cout << "  (none)\n"; // none message
        for (auto &p : prescriptions) cout << "  - " << p << "\n"; // print prescriptions
        bill.printBillSummary(); // delegate to Bill to print billing info
    } // end printFullRecord

private: // internal patient fields
    int id; // unique patient identifier
    string name; // patient name
    int age; // age in years
    string gender; // gender string
    string symptoms; // reported symptoms
    string admissionDate; // admission date string

    vector<string> diagnoses; // recorded diagnoses
    vector<string> medicalNotes; // recorded medical notes
    vector<string> prescriptions; // prescriptions/medications
    Bill bill; // billing object associated with this patient
}; // end class Patient

// User base class models a generic system user; derived roles implement showMenu().
class User { // abstract base class for all user roles
public: // public API shared by all users
    User(const string &username_, const string &password_, Role role_) // constructor
        : username(username_), password(password_), role(role_) {} // initialize fields
    virtual ~User() = default; // virtual destructor for safe polymorphic deletion

    string getUsername() const { return username; } // get username
    Role getRole() const { return role; } // get role enum

    bool checkPassword(const string &pw) const { return pw == password; } // simple password check (plaintext)
    void setPassword(const string &pw) { password = pw; } // change stored password

    virtual void showMenu(HospitalSystem &sys) = 0; // pure virtual: each role provides menu behavior

protected: // accessible to derived classes
    string username; // account username
    string password; // account password (plaintext in this demo)
    Role role; // account role
}; // end class User

// Derived role class declarations; definitions of showMenu appear after HospitalSystem.
class AdminUser : public User { // admin role, can manage users
public:
    AdminUser(const string &username_, const string &password_) // constructor delegates to User
        : User(username_, password_, Role::ADMIN) {} // set role to ADMIN
    void showMenu(HospitalSystem &sys) override; // declare menu function (defined later)
}; // end AdminUser

class NurseUser : public User { // nurse role, registers patients
public:
    NurseUser(const string &username_, const string &password_) // constructor
        : User(username_, password_, Role::NURSE) {} // set role to NURSE
    void showMenu(HospitalSystem &sys) override; // menu declaration
}; // end NurseUser

class DoctorUser : public User { // doctor role, adds diagnoses and billing
public:
    DoctorUser(const string &username_, const string &password_) // constructor
        : User(username_, password_, Role::DOCTOR) {} // set role to DOCTOR
    void showMenu(HospitalSystem &sys) override; // menu declaration
}; // end DoctorUser

class PharmacistUser : public User { // pharmacist role, dispenses meds and updates bill
public:
    PharmacistUser(const string &username_, const string &password_) // constructor
        : User(username_, password_, Role::PHARMACIST) {} // set role to PHARMACIST
    void showMenu(HospitalSystem &sys) override; // menu declaration
}; // end PharmacistUser

class AccountsUser : public User { // accounts manager role, handles payments/bills
public:
    AccountsUser(const string &username_, const string &password_) // constructor
        : User(username_, password_, Role::ACCOUNTS) {} // set role to ACCOUNTS
    void showMenu(HospitalSystem &sys) override; // menu declaration
}; // end AccountsUser

// HospitalSystem manages users, patients, authentication, and program flow.
class HospitalSystem { // central application controller
public:
    HospitalSystem() { // constructor sets up default admin account
        users.push_back(make_shared<AdminUser>("admin", "admin123")); // create default admin user
    } // end constructor

    void run(); // top-level program loop; definition later

    // User management helpers
    bool usernameExists(const string &uname) const { // check for existing username
        for (auto &u : users) if (u->getUsername() == uname) return true; // linear search
        return false; // not found
    } // end usernameExists

    void addUser(shared_ptr<User> user) { users.push_back(move(user)); } // add a new user (move pointer into vector)

    bool deleteUser(const string &username) { // remove a user by username with safety checks
        auto it = find_if(users.begin(), users.end(), // find iterator matching username
            [&](const shared_ptr<User> &u) { return u->getUsername() == username; });
        if (it == users.end()) return false; // not found
        if ((*it)->getRole() == Role::ADMIN) { // if deleting an admin...
            int adminCount = 0; // count admins
            for (auto &u : users) if (u->getRole() == Role::ADMIN) adminCount++; // increment
            if (adminCount <= 1) { // disallow deletion of last admin
                cout << "Cannot delete the last Admin account.\n"; // explain
                return false; // fail deletion
            }
        }
        users.erase(it); // remove user
        return true; // deletion success
    } // end deleteUser

    void listEmployees() const { // print list of registered employees
        cout << "---- Registered Employees ----\n"; // header
        for (auto &u : users) { // iterate users
            cout << "Username: " << u->getUsername() << " | Role: " << roleToString(u->getRole()) << "\n"; // print details
        }
        cout << "------------------------------\n"; // footer
    } // end listEmployees

    shared_ptr<User> authenticate(const string &username, const string &password) { // authenticate credentials
        for (auto &u : users) { // iterate users
            if (u->getUsername() == username && u->checkPassword(password)) return u; // match found, return pointer
        }
        return nullptr; // no match
    } // end authenticate

    // Patient management helpers
    int registerPatient(const string &name, int age, const string &gender,
                        const string &symptoms, const string &date) { // create and store a patient
        int id = ++lastPatientId; // increment id counter
        patients.emplace_back(id, name, age, gender, symptoms, date); // construct Patient in-place
        cout << "Patient registered with ID: " << id << "\n"; // inform user
        return id; // return assigned id
    } // end registerPatient

    Patient* findPatientById(int id) { // find patient and return pointer or nullptr
        for (auto &p : patients) if (p.getId() == id) return &p; // return address within vector
        return nullptr; // not found
    } // end findPatientById

    void listPatientsBrief() const { // print brief list of patients (id + name)
        cout << "---- Patients (brief) ----\n"; // header
        for (auto &p : patients) { cout << "ID: " << p.getId() << " | Name: " << p.getName() << "\n"; } // entries
        cout << "--------------------------\n"; // footer
    } // end listPatientsBrief

    const vector<shared_ptr<User>>& getUsers() const { return users; } // read-only access to users list

private: // internal storage
    vector<shared_ptr<User>> users; // polymorphic user container
    vector<Patient> patients; // in-memory patient records
    int lastPatientId = 0; // monotonic patient ID generator
}; // end class HospitalSystem

// Definitions of showMenu for each role follow, now that HospitalSystem is defined.

// AdminUser menu: manage employee accounts and change own password.
void AdminUser::showMenu(HospitalSystem &sys) { // admin menu loop
    while (true) { // menu repeats until logout
        cout << "\n--- Admin Menu ---\n"; // menu header
        cout << "1. Register employee\n"; // option 1
        cout << "2. Delete employee\n"; // option 2
        cout << "3. View all employees\n"; // option 3
        cout << "4. Change my password\n"; // option 4
        cout << "5. Logout (Back)\n"; // option 5
        cout << "Choose an option: "; // prompt
        int opt = readIntInRange(1,5); // read option safely
        if (opt == 1) { // register new employee
            string uname = readNonEmptyLine("Enter username for employee: "); // ask username
            if (sys.usernameExists(uname)) { cout << "Username already exists.\n"; continue; } // check unique
            cout << "Select role:\n"; // role selection prompt
            cout << "1. Doctor\n2. Nurse\n3. Pharmacist\n4. Accounts Manager\nChoose role: "; // list roles
            int r = readIntInRange(1, 4); // read role choice
            string pw = readNonEmptyLine("Set password for employee: "); // set password
            shared_ptr<User> newUser; // pointer to hold created user
            switch (r) { // create appropriate derived user
                case 1: newUser = make_shared<DoctorUser>(uname, pw); break; // doctor
                case 2: newUser = make_shared<NurseUser>(uname, pw); break; // nurse
                case 3: newUser = make_shared<PharmacistUser>(uname, pw); break; // pharmacist
                case 4: newUser = make_shared<AccountsUser>(uname, pw); break; // accounts manager
                default: break; // unreachable due to validation
            }
            sys.addUser(newUser); // add to system
            cout << "Employee registered: " << uname << " (" << roleToString(newUser->getRole()) << ")\n"; // confirm
        } else if (opt == 2) { // delete employee
            sys.listEmployees(); // show current employees
            string del = readNonEmptyLine("Enter username to delete (or type 'back' to cancel): "); // get username
            if (del == "back") continue; // cancel
            if (!sys.usernameExists(del)) { cout << "No such user.\n"; continue; } // check existence
            if (del == username) { cout << "You cannot delete your own account here.\n"; continue; } // guard self-deletion
            if (sys.deleteUser(del)) cout << "Deleted user: " << del << "\n"; // deletion outcome
            else cout << "Failed to delete user.\n"; // inform failure
        } else if (opt == 3) { // view all employees
            sys.listEmployees(); // list employees
        } else if (opt == 4) { // change own password
            string newpw = readNonEmptyLine("Enter new password: "); // prompt for new pw
            setPassword(newpw); // update stored password
            cout << "Password updated.\n"; // confirm
        } else { // logout/back
            break; // return to caller (HospitalSystem::run)
        }
    } // end admin menu loop
} // end AdminUser::showMenu

// NurseUser menu: register patients and view basic info.
void NurseUser::showMenu(HospitalSystem &sys) { // nurse menu loop
    while (true) { // repeat until logout
        cout << "\n--- Nurse Menu ---\n"; // header
        cout << "1. Register new patient\n"; // option 1
        cout << "2. View basic patient information\n"; // option 2
        cout << "3. Change my password\n"; // option 3
        cout << "4. Logout (Back)\n"; // option 4
        cout << "Choose an option: "; // prompt
        int opt = readIntInRange(1,4); // read choice
        if (opt == 1) { // register patient
            string name = readNonEmptyLine("Full name: "); // get name
            int age; // age var
            while (true) { // validate age input reading
                cout << "Age: "; // prompt age
                if ((cin >> age) && age > 0) { // attempt read and check positive
                    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // clear remainder
                    break; // accept age
                }
                cin.clear(); // clear error state
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // discard line
                cout << "Invalid age.\n"; // inform user
            }
            string gender = readNonEmptyLine("Gender: "); // read gender
            string symptoms = readNonEmptyLine("Symptoms: "); // read symptoms
            string date = readNonEmptyLine("Date of admission (YYYY-MM-DD): "); // read admission date
            sys.registerPatient(name, age, gender, symptoms, date); // register via system
        } else if (opt == 2) { // view basic info
            sys.listPatientsBrief(); // show brief list to choose from
            cout << "Enter patient ID to view (0 to cancel): "; // ask id
            int id = readIntInRange(0, 1000000); // read id, allow 0 to cancel
            if (id == 0) continue; // canceled by user
            Patient* p = sys.findPatientById(id); // find pointer to patient record
            if (!p) { cout << "Patient not found.\n"; continue; } // not found case
            p->printBasicInfo(); // print minimal info
        } else if (opt == 3) { // change password
            string newpw = readNonEmptyLine("Enter new password: "); // prompt
            setPassword(newpw); // update password
            cout << "Password updated.\n"; // confirm
        } else break; // logout/back
    } // end nurse loop
} // end NurseUser::showMenu

// DoctorUser menu: view and update patient records, add billing entries.
void DoctorUser::showMenu(HospitalSystem &sys) { // doctor menu loop
    while (true) { // repeat until logout
        cout << "\n--- Doctor Menu ---\n"; // header
        cout << "1. View registered patient records (brief)\n"; // option 1
        cout << "2. View full patient record by ID\n"; // option 2
        cout << "3. Add diagnostic information\n"; // option 3
        cout << "4. Add medical notes\n"; // option 4
        cout << "5. Prescribe medication\n"; // option 5
        cout << "6. Add billing entry (consultation/tests)\n"; // option 6
        cout << "7. Change my password\n"; // option 7
        cout << "8. Logout (Back)\n"; // option 8
        cout << "Choose an option: "; // prompt
        int opt = readIntInRange(1,8); // read choice
        if (opt == 1) { // list patients
            sys.listPatientsBrief(); // show brief patients list
        } else if (opt == 2) { // view full record by id
            cout << "Enter patient ID (0 to cancel): "; // prompt
            int id = readIntInRange(0, 1000000); // read id
            if (id == 0) continue; // canceled
            Patient* p = sys.findPatientById(id); // find patient
            if (!p) { cout << "Patient not found.\n"; continue; } // not found
            p->printFullRecord(); // print everything including bill
        } else if (opt == 3) { // add diagnosis
            cout << "Enter patient ID: "; // prompt id
            int id = readIntInRange(1, 1000000); // read id
            Patient* p = sys.findPatientById(id); // find patient pointer
            if (!p) { cout << "Patient not found.\n"; continue; } // not found
            string diag = readNonEmptyLine("Enter diagnostic information: "); // get diagnosis text
            p->addDiagnosis(diag); // add diagnosis to patient record
            cout << "Diagnosis added.\n"; // confirm
        } else if (opt == 4) { // add medical note
            cout << "Enter patient ID: "; // prompt
            int id = readIntInRange(1, 1000000); // read id
            Patient* p = sys.findPatientById(id); // locate patient
            if (!p) { cout << "Patient not found.\n"; continue; } // if missing
            string note = readNonEmptyLine("Enter medical note: "); // enter note
            p->addMedicalNote(note); // add to record
            cout << "Medical note added.\n"; // confirm
        } else if (opt == 5) { // prescribe medication
            cout << "Enter patient ID: "; // prompt
            int id = readIntInRange(1, 1000000); // read id
            Patient* p = sys.findPatientById(id); // find patient
            if (!p) { cout << "Patient not found.\n"; continue; } // missing case
            string presc = readNonEmptyLine("Enter prescription details: "); // get prescription text
            p->addPrescription(presc); // add prescription to patient's list
            cout << "Prescription recorded.\n"; // confirm
        } else if (opt == 6) { // add billing charge
            cout << "Enter patient ID: "; // prompt
            int id = readIntInRange(1, 1000000); // read id
            Patient* p = sys.findPatientById(id); // find patient
            if (!p) { cout << "Patient not found.\n"; continue; } // not found
            string desc = readNonEmptyLine("Charge description (e.g., Consultation, X-ray): "); // description
            double amt; // amount variable
            while (true) { // loop until valid amount entered
                cout << "Amount: $"; // prompt for amount
                if ((cin >> amt) && amt > 0.0) { cin.ignore(numeric_limits<streamsize>::max(), '\n'); break; } // valid
                cin.clear(); // clear error state
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // discard line
                cout << "Invalid amount.\n"; // inform
            }
            p->getBill().addCharge(desc, amt); // add charge to patient's bill
            cout << "Charge added to bill.\n"; // confirm
        } else if (opt == 7) { // change own password
            string newpw = readNonEmptyLine("Enter new password: "); // prompt
            setPassword(newpw); // set new password
            cout << "Password updated.\n"; // confirm
        } else break; // logout/back
    } // end doctor loop
} // end DoctorUser::showMenu

// PharmacistUser menu: view records, record dispensed meds, add medication costs.
void PharmacistUser::showMenu(HospitalSystem &sys) { // pharmacist menu
    while (true) { // loop until exit
        cout << "\n--- Pharmacist Menu ---\n"; // header
        cout << "1. View patient medical record (full)\n"; // option 1
        cout << "2. Record medication dispensed\n"; // option 2
        cout << "3. Add medication cost to patient bill\n"; // option 3
        cout << "4. Change my password\n"; // option 4
        cout << "5. Logout (Back)\n"; // option 5
        cout << "Choose an option: "; // prompt
        int opt = readIntInRange(1,5); // read choice
        if (opt == 1) { // view full record
            cout << "Enter patient ID: "; // prompt
            int id = readIntInRange(1, 1000000); // read id
            Patient* p = sys.findPatientById(id); // find patient
            if (!p) { cout << "Patient not found.\n"; continue; } // missing
            p->printFullRecord(); // print full patient info
        } else if (opt == 2) { // record medication dispensed
            cout << "Enter patient ID: "; // prompt
            int id = readIntInRange(1, 1000000); // read id
            Patient* p = sys.findPatientById(id); // find patient
            if (!p) { cout << "Patient not found.\n"; continue; } // not found
            string med = readNonEmptyLine("Enter medication details dispensed: "); // read medication info
            p->addPrescription(med); // store dispensed med as prescription (simple model)
            cout << "Medication dispensed and recorded.\n"; // confirm
        } else if (opt == 3) { // add medication cost to bill
            cout << "Enter patient ID: "; // prompt
            int id = readIntInRange(1, 1000000); // read id
            Patient* p = sys.findPatientById(id); // find patient
            if (!p) { cout << "Patient not found.\n"; continue; } // not found
            string desc = readNonEmptyLine("Medication description: "); // get description
            double amt; // amount variable
            while (true) { // validate amount entry
                cout << "Amount: $"; // prompt
                if ((cin >> amt) && amt > 0.0) { cin.ignore(numeric_limits<streamsize>::max(), '\n'); break; } // accept
                cin.clear(); // clear errors
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // discard
                cout << "Invalid amount.\n"; // inform
            }
            p->getBill().addCharge(desc, amt); // add charge to bill
            cout << "Medication cost added to bill.\n"; // confirm
        } else if (opt == 4) { // change password
            string newpw = readNonEmptyLine("Enter new password: "); // prompt for new password
            setPassword(newpw); // update password
            cout << "Password updated.\n"; // confirm
        } else break; // logout/back
    } // end pharmacist loop
} // end PharmacistUser::showMenu

// AccountsUser menu: view bills, record payments, and set status manually.
void AccountsUser::showMenu(HospitalSystem &sys) { // accounts manager menu
    while (true) { // loop until logout
        cout << "\n--- Accounts Manager Menu ---\n"; // header
        cout << "1. View complete patient bill\n"; // option 1
        cout << "2. Record payment made\n"; // option 2
        cout << "3. Mark bill status manually\n"; // option 3
        cout << "4. Change my password\n"; // option 4
        cout << "5. Logout (Back)\n"; // option 5
        cout << "Choose an option: "; // prompt
        int opt = readIntInRange(1,5); // read selection
        if (opt == 1) { // view bill
            cout << "Enter patient ID: "; // prompt
            int id = readIntInRange(1, 1000000); // read id
            Patient* p = sys.findPatientById(id); // find patient
            if (!p) { cout << "Patient not found.\n"; continue; } // not found
            p->getBill().printBillSummary(); // show bill summary
        } else if (opt == 2) { // record payment
            cout << "Enter patient ID: "; // prompt
            int id = readIntInRange(1, 1000000); // read id
            Patient* p = sys.findPatientById(id); // find patient
            if (!p) { cout << "Patient not found.\n"; continue; } // not found
            string method = readNonEmptyLine("Payment method (e.g., Cash/Card/Insurance): "); // read payment method
            double amt; // amount variable
            while (true) { // validate amount entry
                cout << "Amount paid: $"; // prompt
                if ((cin >> amt) && amt > 0.0) { cin.ignore(numeric_limits<streamsize>::max(), '\n'); break; } // accept
                cin.clear(); // clear errors
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // discard line
                cout << "Invalid amount.\n"; // inform
            }
            p->getBill().addPayment(method, amt); // record payment on bill
            cout << "Payment recorded.\n"; // confirm
        } else if (opt == 3) { // manually change bill status
            cout << "Enter patient ID: "; // prompt
            int id = readIntInRange(1, 1000000); // read id
            Patient* p = sys.findPatientById(id); // find patient
            if (!p) { cout << "Patient not found.\n"; continue; } // missing
            cout << "Select status:\n1. Fully cleared\n2. Partially paid\n3. Pending\nChoose: "; // status options
            int s = readIntInRange(1,3); // read status selection
            Bill::Status ns; // new status variable
            switch (s) { // map input to enum
                case 1: ns = Bill::Status::FULLY_CLEARED; break; // fully cleared
                case 2: ns = Bill::Status::PARTIALLY_PAID; break; // partially paid
                default: ns = Bill::Status::PENDING; break; // pending fallback
            }
            p->getBill().setStatus(ns); // set bill status
            cout << "Bill status updated.\n"; // confirm
        } else if (opt == 4) { // change own password
            string newpw = readNonEmptyLine("Enter new password: "); // prompt
            setPassword(newpw); // update stored password
            cout << "Password updated.\n"; // confirm
        } else break; // logout/back
    } // end accounts loop
} // end AccountsUser::showMenu

// Implementation of HospitalSystem::run which provides the startup/login menu and dispatch.
void HospitalSystem::run() { // main application loop
    while (true) { // loop until user chooses Exit
        cout << "\n=== Hospital Management System ===\n"; // top-level header
        cout << "1. Login\n"; // login option
        cout << "2. Exit\n"; // exit option
        cout << "Choose an option: "; // prompt
        int opt = readIntInRange(1,2); // read choice
        if (opt == 2) { // exit selected
            cout << "Exiting. Goodbye.\n"; // farewell
            break; // break main loop to exit
        }
        // handle login
        string uname = readNonEmptyLine("Username: "); // ask username
        string pw = readNonEmptyLine("Password: "); // ask password
        auto user = authenticate(uname, pw); // attempt authentication
        if (!user) { cout << "Invalid username or password.\n"; continue; } // auth failed, loop again
        cout << "Login successful. Welcome, " << user->getUsername() << " (" << roleToString(user->getRole()) << ")\n"; // greet user
        user->showMenu(*this); // polymorphic dispatch to role-specific menu
        cout << "Logged out.\n"; // after logout from role menu
    } // end while
} // end HospitalSystem::run

// Program entry point: create HospitalSystem and start run loop.
int main() { // main function
    HospitalSystem hs; // instantiate system (creates default admin)
    cout << "Default admin account created: username='admin', password='admin123'\n"; // inform about default admin
    hs.run(); // start main interactive loop
    return 0; // return success
} // end main