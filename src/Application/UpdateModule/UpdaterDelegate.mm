#import "UpdaterDelegate.h"
#import "SparkleAutoUpdater.h"

#include <QMessageBox>

@implementation UpdaterDelegate

- (void)setCPPUpdater:(SparkleAutoUpdater*)u
{
    cppUpdater = u;
}

/* SUUpdater delegate methods 
 (the documentation below reflects the one in https://github.com/andymatuschak/Sparkle/wiki/customization )

// Use this to override the default behavior for Sparkle prompting the
// user about automatic update checks. You could use this to make Sparkle
// prompt for permission on the first launch instead of the second.
- (BOOL)updaterShouldPromptForPermissionToCheckForUpdates:(SUUpdater *)bundle
{
}

- (void)updater:(SUUpdater *)updater didFinishLoadingAppcast:(SUAppcast *)appcast
{
}

// If you're using special logic or extensions in your appcast, implement
// this to use your own logic for finding a valid update, if any, in the given appcast.
- (SUAppcastItem *)bestValidUpdateInAppcast:(SUAppcast *)appcast
                                 forUpdater:(SUUpdater *)bundle
{
}

- (void)updater:(SUUpdater *)updater didFindValidUpdate:(SUAppcastItem *)update
{
}

- (void)updaterDidNotFindUpdate:(SUUpdater *)update
{
    QMessageBox *message = new QMessageBox(QMessageBox::Information, "Check for update", "You already have the latest version", QMessageBox::Ok);
    message->setWindowModality(Qt::NonModal);
    message->exec();
}

// Sent immediately before installing the specified update.
- (void)updater:(SUUpdater *)updater willInstallUpdate:(SUAppcastItem *)update
{
}

// Return YES to delay the relaunch until you do some processing.
// Invoke the provided NSInvocation to continue the relaunch.
- (BOOL)updater:(SUUpdater *)updater
shouldPostponeRelaunchForUpdate:(SUAppcastItem *)update
  untilInvoking:(NSInvocation *)invocation
{
}


 // Some apps *can not* be relaunched in certain circumstances. They can use this method
 //	to prevent a relaunch "hard":
- (BOOL)updaterShouldRelaunchApplication:(SUUpdater *)updater
{
}


// Called immediately before relaunching.
- (void)updaterWillRelaunchApplication:(SUUpdater *)updater
{
}

// This method allows you to provide a custom version comparator.
// If you don't implement this method or return nil, the standard version
// comparator will be used. See SUVersionComparisonProtocol.h for more.
- (id <SUVersionComparison>)versionComparatorForUpdater:(SUUpdater *)updater
{
}

// Returns the path which is used to relaunch the client after the update
// is installed. By default, the path of the host bundle.
- (NSString *)pathToRelaunchForUpdater:(SUUpdater *)updater
{
}

// This method allows you to add extra parameters to the appcast URL,
// potentially based on whether or not Sparkle will also be sending along
// the system profile. This method should return an array of dictionaries
// with keys: "key", "value", "displayKey", "displayValue", the latter two
// being human-readable variants of the former two.
- (NSArray *)feedParametersForUpdater:(SUUpdater *)updater
                 sendingSystemProfile:(BOOL)sendingProfile
{
}


 // Called before and after, respectively, an updater shows a modal alert window, to give the host
 //	the opportunity to hide attached windows etc. that may get in the way:
 -(void)	updaterWillShowModalAlert:(SUUpdater *)updater
{
}

 -(void)	updaterDidShowModalAlert:(SUUpdater *)updater
{
}
*/

@end


