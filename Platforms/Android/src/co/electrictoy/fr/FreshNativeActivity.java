package co.electrictoy.fr;

import android.app.NativeActivity;
import android.content.Intent;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings.Secure;
import android.util.Log;
import android.view.View;


// TODO reinstate license checking throughout

//import com.google.android.vending.licensing.AESObfuscator;
//import com.google.android.vending.licensing.LicenseChecker;
//import com.google.android.vending.licensing.LicenseCheckerCallback;
//import com.google.android.vending.licensing.ServerManagedPolicy;

public abstract class FreshNativeActivity extends NativeActivity implements /* TODO LicenseCheckerCallback, */ AudioManager.OnAudioFocusChangeListener
{
	private static String TAG = "Fresh";

//    private LicenseChecker mChecker;

	// Override per game.
	//
	protected abstract String BASE64_PUBLIC_KEY();
	protected abstract byte[] SALT();
	protected abstract boolean enableLicenseChecking();

	// Callable native functions
	//
	private static native void onUnauthorizedPlayDetected();
	private static native void onAudioFocusGained();
	private static native void onAudioFocusLost();


	private void setImmersiveSticky()
	{
	    View decorView = getWindow().getDecorView();
	    decorView.setSystemUiVisibility(
				  View.SYSTEM_UI_FLAG_FULLSCREEN
	            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
	            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
	            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
	            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
	            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE );
	}

	public void visitURL( String url )
	{
		Log.v( TAG, "Visiting URL '" + url + "'." );
		Intent intent = new Intent( Intent.ACTION_VIEW );
		intent.setData( Uri.parse( url ));
		startActivity( intent );
	}

	//
	// License checking Support
	//

	@Override
    public void onCreate( Bundle savedInstanceState )
	{
		super.onCreate( savedInstanceState );

		//
		// Enable license checking if desired.
		//

		final String publicKey = BASE64_PUBLIC_KEY();
		final byte[] salt = SALT();

		if( enableLicenseChecking() && publicKey.length() > 0 && salt.length > 0 )
		{
			// TODO
//			//
//			// Establish licensing credentials for the user.
//			//
//			String deviceId = Secure.getString( getContentResolver(), Secure.ANDROID_ID );
//
//			// Construct the LicenseChecker with a policy.
//			mChecker = new LicenseChecker(
//					this, new ServerManagedPolicy( this,
//					new AESObfuscator( salt, getPackageName(), deviceId ) ),
//					publicKey );
//
//			mChecker.checkAccess( this );
	    }
		else
		{
			Log.d( TAG, "License checking disabled." );
		}

		//
		// Setup system audio.
		//

		// Ensure that the volume buttons control our audio stream ("music" === "game").
		//
		setVolumeControlStream( AudioManager.STREAM_MUSIC );
    }

	@Override
	public void onResume()
	{
		//
		// Move to immersive full screen mode. See http://developer.android.com/training/system-ui/immersive.html
		//
		setImmersiveSticky();
		setupAudioFocus();
		super.onResume();
	}

	@Override
	protected void onPause()
	{
		AudioManager audioManager = (AudioManager) getSystemService( AUDIO_SERVICE );
		audioManager.abandonAudioFocus( this );

		super.onPause();
	}

	// Licensing callbacks.
	//
	public void allow( int policyReason )
	{
		if( isFinishing() )
		{
			// Don't update UI if Activity is finishing.
			return;
		}

		Log.v( TAG, "License allowed." );
	}

	public void dontAllow( int policyReason )
	{
		Log.v( TAG, "License not allowed. Policy reason: " + policyReason );

		// Go to the market immediately.
		//
		visitURL( "market://details?id=" + getPackageName() );

		// Stop the running game.
		//
		stopNativeApp();
	}

	public void applicationError( int errorCode )
	{
		Log.v( TAG, "License error. " + Integer.toString( errorCode ));
	}

	protected void stopNativeApp()
	{
		onUnauthorizedPlayDetected();
	}

    @Override
    protected void onDestroy()
	{
        super.onDestroy();
        // TODO
//        mChecker.onDestroy();
    }

	protected void setupAudioFocus()
	{
		// Setup AudioFocus system (http://developer.android.com/training/managing-audio/audio-focus.html)
		//
		AudioManager audioManager = (AudioManager) getSystemService( AUDIO_SERVICE );
		int result = audioManager.requestAudioFocus( this,
										   AudioManager.STREAM_MUSIC,
										   AudioManager.AUDIOFOCUS_GAIN );
	}


	@Override
	public void onAudioFocusChange( int focusChange )
	{
		AudioManager audioManager = (AudioManager) getSystemService( AUDIO_SERVICE );

		switch( focusChange )
		{
			case AudioManager.AUDIOFOCUS_GAIN:
				onAudioFocusGained();
				break;

			case AudioManager.AUDIOFOCUS_LOSS:
				onAudioFocusLost();
				break;

			case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
			case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
				onAudioFocusLost();
				break;

			default:
				Log.w( TAG, "Received unexpected focusChange: " + focusChange );
				break;
		}
	}


	@Override
	public void onBackPressed()
	{
		moveTaskToBack(true);
	}
}
