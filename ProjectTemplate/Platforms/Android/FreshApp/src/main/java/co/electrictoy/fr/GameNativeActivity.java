package fresh.TODO.domain.freshshortname;

import co.electrictoy.fr.FreshNativeActivity;

public class GameNativeActivity extends FreshNativeActivity
{
	@Override
	protected String BASE64_PUBLIC_KEY()
	{
		// ** TODO ** Modify per game.
        return "";
	}

	@Override
	protected byte[] SALT()
	{
		// ** TODO ** Modify per game.
	    return new byte[] {
			
		};
	}

	@Override
	protected boolean enableLicenseChecking()
	{
		// TODO!!! Set to true at release.
		return false;
	}
	
	static
	{
		System.loadLibrary( "freshshortname" );
	}	
}
