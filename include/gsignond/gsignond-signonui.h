#ifndef _GSIGNOND_SIGNONUI_H_
#define _GSIGNOND_SIGNONUI_H_

/**
 * @GSignondSignonuiError:
 * @SIGNONUI_ERROR_NONE: No errors
 * @SIGNONUI_ERROR_GENERAL: Generic error during interaction
 * @SIGNONUI_ERROR_NO_SIGNONUI: Cannot send request to signon-ui
 * @SIGNONUI_ERROR_BAD_PARAMETERS:Signon-Ui cannot create dialog based on the given UiSessionData
 * @SIGNONUI_ERROR_CANCELED: User canceled action. Plugin should not retry automatically after this
 * @SIGNONUI_ERROR_NOT_AVAILABLE: Requested ui is not available. For example browser cannot be started
 * @SIGNONUI_ERROR_BAD_URL: Given url was not valid
 * @SIGNONUI_ERROR_BAD_CAPTCHA: Given captcha image was not valid
 * @SIGNONUI_ERROR_BAD_CAPTCHA_URL: Given url for capctha loading was not valid
 * @SIGNONUI_ERROR_REFRESH_FAILED: Refresh failed
 * @SIGNONUI_ERROR_FORBIDDEN: Showing ui forbidden by ui policy
 * @SIGNONUI_ERROR_FORGOT_PASSWORD: User pressed forgot password
 */
typedef enum {
    SIGNONUI_ERROR_NONE = 0, 
    SIGNONUI_ERROR_GENERAL,
    SIGNONUI_ERROR_NO_SIGNONUI,
    SIGNONUI_ERROR_BAD_PARAMETERS,
    SIGNONUI_ERROR_CANCELED,
    SIGNONUI_ERROR_NOT_AVAILABLE, 
    SIGNONUI_ERROR_BAD_URL, 
    SIGNONUI_ERROR_BAD_CAPTCHA,
    SIGNONUI_ERROR_BAD_CAPTCHA_URL,
    SIGNONUI_ERROR_REFRESH_FAILED, 
    SIGNONUI_ERROR_FORBIDDEN,
    SIGNONUI_ERROR_FORGOT_PASSWORD
} GSignondSignonuiError;

#endif //_GSIGNOND_SIGNONUI_H_
