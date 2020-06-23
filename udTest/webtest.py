from selenium import webdriver
from selenium.webdriver.chrome.options import Options as ChromeOptions
from selenium.webdriver.firefox.options import Options as FirefoxOptions
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.common.exceptions import TimeoutException

import sys

if len(sys.argv) > 1 and sys.argv[1] == 'firefox':
	firefox_options = FirefoxOptions()
	firefox_options.headless = True
	firefox_options.set_preference("javascript.options.shared_memory", True)

	# Only available in firefox-trunk
	firefox_options.set_preference("dom.postMessage.sharedArrayBuffer.bypassCOOP_COEP.insecure.enabled", True)

	driver = webdriver.Firefox(firefox_binary="/usr/bin/firefox-trunk", options=firefox_options)
elif len(sys.argv) > 1 and sys.argv[1] == 'chrome':
	chrome_options = ChromeOptions()
	chrome_options.add_argument("--disable_gpu")
	chrome_options.add_argument("--headless")
	chrome_options.add_argument("--no-sandbox")

	driver = webdriver.Chrome(executable_path="/chromedriver", options=chrome_options)
else:
	print("Run either:")
	print("\twebtest.py chrome")
	print("\twebtest.py firefox")
	exit(-1)

start_url = "http://127.0.0.1:8000/udTest.html"
driver.get(start_url)

try:
	element = WebDriverWait(driver, 30).until(EC.text_to_be_present_in_element_value((By.ID, 'output'), ' tests.'))
except TimeoutException:
	print("Loading took too much time!")
except Exception as e:
	print("Something went wrong! ", e)

# TODO: Output changes as they come in...
print(driver.execute_script("return document.getElementById('output').value;"))

exitStatus = driver.execute_script("return EXITSTATUS;")

driver.quit()

exit(exitStatus)
