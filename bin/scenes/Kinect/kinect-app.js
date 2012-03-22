
var _p = null;
var _lc = "[KinectApp]: ";

function KinectClient() 
{
    // Add some other scene for assets so we dont need to make unwanted copies.
    // Even if the folder would not be there, its just for material so not critical.
    asset.DeserializeAssetStorageFromString("name=PhysicsScene;type=LocalAssetStorage;src=" + application.installationDirectory + "scenes/Physics", false);
    
    this.data = {};
    this.data.test = "moi test";
    this.data.bonesCreated = false;

    kinect.VideoUpdate.connect(this, this.onVideoAvailable);
    kinect.DepthUpdate.connect(this, this.onDepthAvailable);
    kinect.TrackingSkeletons.connect(this, this.onTrackingSkeletons);
    kinect.SkeletonUpdate.connect(this, this.onSkeletonUpdate);

    if (kinect.HasKinect()) 
    {
        if (!kinect.IsStarted())
        {
            if (!kinect.StartKinect())
                console.LogError(_lc + "Failed to start Kinect");
            else
                console.LogInfo(_lc + "Kinect started");
        }
    } 
    else 
    {
        console.LogWarning(_lc + "No kinect devices available, could not start Kinect!");
    }
    
    // Move freecam so we can see the box skeleton
    var freeLookCam = scene.GetEntityByName("FreeLookCamera");
    if (freeLookCam)
    {
        var t = freeLookCam.placeable.transform;
        t.pos = new float3(0, -40, -200);
        t.rot = new float3(0, -180, 0);
        freeLookCam.placeable.transform = t;
    }
    
    // Remove app ents so we can do live reloads and everything works.
    var boneEnts = scene.GetEntitiesWithComponent("EC_DynamicComponent", "KinectBone");
    for (var i = 0; i < boneEnts.length; ++i) 
        scene.RemoveEntity(boneEnts[i].id);
    this.data.bonesCreated = false;
}

KinectClient.prototype.shutDown = function () 
{
}

KinectClient.prototype.onVideoAvailable = function () 
{
    // If you want to show the video image use the data getter.
    // var videoImage = kinect.VideoImage();
}

KinectClient.prototype.onDepthAvailable = function () 
{
    // If you want to show the depth image use the data getter.
    // var depthImage = kinect.DepthImage();
}

KinectClient.prototype.onTrackingSkeletons = function (tracking) 
{
    if (!tracking) 
    {
        console.LogInfo(_lc + "Skeleton tracking lost, removing 'bone' boxes");
        var boneEnts = scene.GetEntitiesWithComponent("EC_DynamicComponent", "KinectBone");
        for (var i = 0; i < boneEnts.length; ++i) 
            scene.RemoveEntity(boneEnts[i].id);
        this.data.bonesCreated = false;
    }
}

KinectClient.prototype.onSkeletonUpdate = function (skeleton) 
{
    profiler.BeginBlock("KinectApp_SkeletonUpdate");
    
    // For this example purpouse we only handle skeleton 0.
    if (skeleton.enrollmentIndex != 0)
    {
        profiler.EndBlock();
        return;
    }
    
    if (!this.data.bonesCreated)
    {
        this.data.bonesCreated = true;
        
        console.LogInfo(_lc + "Skeletons detected, creating 'bone' boxes");
        var boneNames = skeleton.boneNames;
        for (var i = 0; i < boneNames.length; ++i) 
        {
            var ent = scene.CreateEntity(scene.NextFreeIdLocal(), ["EC_Name", "EC_Placeable", "EC_Mesh"]);
            ent.temporary = true;
            ent.name = "Kinect_Bone_" + boneNames[i];
            
            

            // Mark the entity so we can easily remove them
            ent.GetOrCreateComponent("EC_DynamicComponent", "KinectBone");
            
            var pos4 = skeleton.BonePosition(boneNames[i]).Mul(100);
            ent.placeable.SetPosition(pos4.xyz());
            
            if (boneNames[i].indexOf("head") != -1)
            {
                // Special face texture for head bone
                ent.mesh.meshRef = "local://plane.mesh";
                ent.mesh.meshMaterial = new AssetReference("local://face.material");
                var mesht = ent.mesh.nodeTransformation;
                mesht.rot = new float3(-90, 0, 180);
                ent.mesh.nodeTransformation = mesht;
                
                ent.placeable.SetScale(25, 25, 25);
            }
            else
            {
                ent.mesh.meshRef = "local://box.mesh";
                ent.mesh.meshMaterial = new AssetReference("local://box.material");
                ent.placeable.SetScale(8, 12, 8);
            }
        }

        profiler.EndBlock();
        return;
    }
    
    // Update position for bones
    var boneNames = skeleton.boneNames;
    for (var i = 0; i < boneNames.length; ++i) 
    {
        var boneName = boneNames[i];

        var ent = scene.GetEntityByName("Kinect_Bone_" + boneName);
        if (!ent)
            return;
        
        if (!skeleton.IsBoneTracked(boneName))
        {
            ent.placeable.visible = false;
            continue;
        }

        if (!ent.placeable.visible)
            ent.placeable.visible = true;
        
        var pos4 = skeleton.BonePosition(boneNames[i]).Mul(100);
        ent.placeable.SetPosition(pos4.xyz());
    }
    
    profiler.EndBlock();
}

if (!server.IsRunning() && !framework.IsHeadless()) 
{
    if (kinect != null)
        _p = new KinectClient();
    else
        console.LogError(_lc + "KinectModule is not loaded, cannot proceed with Kinect example application!");
} 
else 
{
    if (server.IsRunning())
        console.LogError(_lc + "Not proceeding with Kinect example application, running in server instance!");
    else if (framework.IsHeadless())
        console.LogError(_lc + "Not proceeding with Kinect example application, running in headless Tundra!");
}

function OnScriptDestroyed() 
{
    if (_p) 
    {
        _p.shutDown();
        _p = null;
    }
}