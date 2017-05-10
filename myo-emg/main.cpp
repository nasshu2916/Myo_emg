#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <myo/myo.hpp>

#include <iostream>
#include <chrono>

class DataCollector : public myo::DeviceListener {
public:
    DataCollector()
    : emgSamples(),currentPose(),onArm(false),isUnlocked(false)
    {
    }
    
    // onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user.
    void onUnpair(myo::Myo* myo, uint64_t timestamp)
    {
        // We've lost a Myo.
        // Let's clean up some leftover state.
        emgSamples.fill(0);
    }
    
    // onEmgData() is called whenever a paired Myo has provided new EMG data, and EMG streaming is enabled.
    void onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg)
    {
        for (int i = 0; i < 8; i++) {
            emgSamples[i] = emg[i];
        }
    }
    
    // onPose() is called whenever the Myo detects that the person wearing it has changed their pose, for example,
    // making a fist, or not making a fist anymore.
    void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose)
    {
        currentPose = pose;
        
        if (pose != myo::Pose::unknown && pose != myo::Pose::rest) {
            // Tell the Myo to stay unlocked until told otherwise. We do that here so you can hold the poses without the
            // Myo becoming locked.
            myo->unlock(myo::Myo::unlockHold);
            
            // Notify the Myo that the pose has resulted in an action, in this case changing
            // the text on the screen. The Myo will vibrate.
            myo->notifyUserAction();
        } else {
            // Tell the Myo to stay unlocked only for a short period. This allows the Myo to stay unlocked while poses
            // are being performed, but lock after inactivity.
            myo->unlock(myo::Myo::unlockTimed);
        }
    }
    
    // onArmSync() is called whenever Myo has recognized a Sync Gesture after someone has put it on their
    // arm. This lets Myo know which arm it's on and which way it's facing.
    void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection, float rotation,
                   myo::WarmupState warmupState)
    {
        onArm = true;
        whichArm = arm;
    }
    
    // onArmUnsync() is called whenever Myo has detected that it was moved from a stable position on a person's arm after
    // it recognized the arm. Typically this happens when someone takes Myo off of their arm, but it can also happen
    // when Myo is moved around on the arm.
    void onArmUnsync(myo::Myo* myo, uint64_t timestamp)
    {
        onArm = false;
    }
    
    // onUnlock() is called whenever Myo has become unlocked, and will start delivering pose events.
    void onUnlock(myo::Myo* myo, uint64_t timestamp)
    {
        isUnlocked = true;
    }
    
    // onLock() is called whenever Myo has become locked. No pose events will be sent until the Myo is unlocked again.
    void onLock(myo::Myo* myo, uint64_t timestamp)
    {
        isUnlocked = false;
    }
    
    
    
    // There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData().
    // For this example, the functions overridden above are sufficient.
    
    // We define this function to print the current values that were updated by the on...() functions above.
    void print()
    {
        // Clear the current line
        std::cout << '\r';
        
        // Print out the EMG data.
        for (size_t i = 0; i < emgSamples.size(); i++) {
            std::ostringstream oss;
            
            oss << static_cast<int>(emgSamples[i]);
            std::string emgString = oss.str();
            
            //            std::cout << '[' << emgString << std::string(4 - emgString.size(), ' ') << ']';
            std::cout << emgString << std::string(4 - emgString.size(), ' ') << ',';
            
            
        }
        
        if (onArm) {
            std::cout << '\r';
            // Print out the lock state, the currently recognized pose, and which arm Myo is being worn on.
            
            // Pose::toString() provides the human-readable name of a pose. We can also output a Pose directly to an
            // output stream (e.g. std::cout << currentPose;). In this case we want to get the pose name's length so
            // that we can fill the rest of the field with spaces below, so we obtain it as a string using toString().
            std::string poseString = currentPose.toString();
            
            std::cout << '[' << (isUnlocked ? "unlocked" : "locked  ") << ']'
            << '[' << (whichArm == myo::armLeft ? "L" : "R") << ']'
            << '[' << poseString << std::string(14 - poseString.size(), ' ') << ']';
            if (poseString == "fist") {
                printf("aaaaa");
            }
            
        } else {
            // Print out a placeholder for the arm and pose when Myo doesn't currently know which arm it's on.
            std::cout << '[' << std::string(8, ' ') << ']' << "[?]" << '[' << std::string(14, ' ') << ']';
        }
        
        
        std::cout << std::flush;
    }
    
    //手の値を返す
    int getPose(){
        if (onArm) {
            std::string poseString = currentPose.toString();
            if (poseString == "fist") {
                return 1;
            }else if (poseString == "fingersSpread"){
                return 2;
            }
        }
        return 0;
        
    }
    
    
    bool onArm;
    myo::Arm whichArm;
    bool isUnlocked;
    // The values of this array is set by onEmgData() above.
    std::array<int8_t, 8> emgSamples;
    myo::Pose currentPose;
};



int main(int argc, char** argv)
{
    
    // We catch any exceptions that might occur below -- see the catch statement for more details.
    try {
        
        // First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
        // publishing your application. The Hub provides access to one or more Myos.
        myo::Hub hub("com.example.emg-data-sample");
        
        std::cout << "Attempting to find a Myo..." << std::endl;
        
        // Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo
        // immediately.
        // waitForMyo() takes a timeout value in milliseconds. In this case we will try to find a Myo for 10 seconds, and
        // if that fails, the function will return a null pointer.
        myo::Myo* myo = hub.waitForMyo(10000);
        
        // If waitForMyo() returned a null pointer, we failed to find a Myo, so exit with an error message.
        if (!myo) {
            throw std::runtime_error("Unable to find a Myo!");
        }
        
        // We've found a Myo.
        std::cout << "Connected to a Myo armband!" << std::endl << std::endl;
        
        // Next we enable EMG streaming on the found Myo.
        myo->setStreamEmg(myo::Myo::streamEmgEnabled);
        
        // Next we construct an instance of our DeviceListener, so that we can register it with the Hub.
        DataCollector collector;
        
        // Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
        // Hub::run() to send events to all registered device listeners.
        hub.addListener(&collector);
        
        std::chrono::system_clock::time_point  start, end; // 型は auto で可
        std::array<int16_t, 6>elapsed;
        
        printf("グーにして下さい\n");
        while (1) {
            
            // In each iteration of our main loop, we run the Myo event loop for a set number of milliseconds.
            // In this case, we wish to update our display 50 times a second, so we run for 1000/20 milliseconds.
            hub.run(1000/20);
            // After processing events, we call the print() member function we defined above to print out the values we've
            // obtained from any events that have occurred.
            
            // collector.print();
            if(collector.getPose() == 1)
                break;
            
        }
        
        for (int i = 0; i < 3; i ++) {
            
            start = std::chrono::system_clock::now(); // 計測開始時間
            printf("10秒後 パー にして下さい\n\n");
            while (1) {
                hub.run(1000/20);
                if(collector.getPose() == 2)
                    break;
                
            }
            end = std::chrono::system_clock::now();  // 計測終了時間
            elapsed[i * 2] = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count(); //処理に要した時間をミリ秒に変換
            
            printf("10秒後 グー にして下さい\n\n");
            start = std::chrono::system_clock::now(); // 計測開始時間
            while (1) {
                hub.run(1000/20);
                if(collector.getPose() == 1)
                    break;
            }
            end = std::chrono::system_clock::now();  // 計測終了時間
            elapsed[i * 2 + 1] = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count(); //処理に要した時間をミリ秒に変換
        }
        
        float time;
        for (int i = 0; i < elapsed.size(); i++) {
            time = (float)elapsed[i] / 1000;
            std::cout << i + 1 << "回目は" << time << " 秒\n";
        }
        std::cout << "でした\n";
        std::cout << std::flush;
        
        
        // If a standard exception occurred, we print out its message and exit.
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Press enter to continue.";
        std::cin.ignore();
        return 1;
    }
}




