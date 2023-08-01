from selenium import webdriver
from selenium.webdriver.chrome.service import Service as ChromeService
from selenium.webdriver.chrome.options import Options as ChromeOptions
from selenium.webdriver.firefox.service import Service as FirefoxService
from selenium.webdriver.firefox.options import Options as FirefoxOptions
from selenium.webdriver.edge.service import Service as EdgeService
from selenium.webdriver.edge.options import Options as EdgeOptions
from selenium.webdriver.safari.service import Service as SafariService
from selenium.webdriver.safari.options import Options as SafariOptions
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.common.exceptions import TimeoutException

import sys
import platform
import os

if len(sys.argv) > 1 and sys.argv[1] == 'firefox':
	firefox_options = FirefoxOptions()
	firefox_options.add_argument("-headless")
	firefox_options.set_preference("javascript.options.shared_memory", True)

	driverpath = os.environ.get('GECKOWEBDRIVER')
	if driverpath is None:
		driverpath = ""

	if platform.system() == "Windows":
		service = FirefoxService(executable_path=driverpath + "\geckodriver.exe")
	else:
		service = FirefoxService(executable_path=driverpath + "/geckodriver")
	driver = webdriver.Firefox(service=service, options=firefox_options)
elif len(sys.argv) > 1 and sys.argv[1] == 'chrome':
	chrome_options = ChromeOptions()
	chrome_options.add_argument("--headless=new")
	chrome_options.add_argument("--remote-debugging-port=9230")

	driverpath = os.environ.get('CHROMEWEBDRIVER')
	if driverpath is None:
		driverpath = ""

	if platform.system() == "Windows":
		service = ChromeService(executable_path=driverpath + "\chromedriver.exe")
	else:
		service = ChromeService(executable_path=driverpath + "/chromedriver")
	driver = webdriver.Chrome(service=service, options=chrome_options)
elif len(sys.argv) > 1 and sys.argv[1] == 'chrome':
	chrome_options = ChromeOptions()
	chrome_options.add_argument("--headless=new")
	chrome_options.add_argument("--remote-debugging-port=9230")

	driverpath = os.environ.get('EDGEWEBDRIVER')
	if driverpath is None:
		driverpath = ""

	if platform.system() == "Windows":
		service = EdgeService(executable_path=driverpath + "\msedgedriver.exe")
	else:
		service = EdgeService(executable_path=driverpath + "/msedgedriver")
	driver = webdriver.Edge(service=service, options=edge_options)
elif len(sys.argv) > 1 and sys.argv[1] == 'safari':
	driver = webdriver.Safari()
elif len(sys.argv) > 1 and sys.argv[1] == 'safaritp':
	safari_options = SafariOptions()
	safari_options.use_technology_preview = True
	service = SafariService(executable_path='/Applications/Safari Technology Preview.app/Contents/MacOS/safaridriver')
	driver = webdriver.Safari(service=service, options=safari_options)
elif len(sys.argv) > 2 and sys.argv[1] == 'remote':
	driver = webdriver.Remote(sys.argv[2])
else:
	print("Run one of the following:")
	print("\twebtest.py chrome")
	print("\twebtest.py firefox")
	print("\twebtest.py edge")
	print("\twebtest.py safari")
	print("\twebtest.py safaritp")
	print("\twebtest.py remote <address>")
	exit(-1)

start_url = "http://localhost:8000/udTest.html"
driver.get(start_url)
success = False

try:
	element = WebDriverWait(driver, 30).until(EC.text_to_be_present_in_element_value((By.ID, 'output'), ' tests.'))
	success = True
except TimeoutException:
	print("Loading took too much time!")
except Exception as e:
	print("Something went wrong! ", e)

# TODO: Output changes as they come in...
print(driver.execute_script("return document.getElementById('output').value;"))

if success:
	element = driver.find_element(By.ID, 'output')
	if "[  FAILED  ]" in element.get_attribute('value'):
		exitStatus = -1
	else:
		exitStatus = driver.execute_script("return EXITSTATUS;")
else:
	exitStatus = -1

driver.quit()

exit(exitStatus)
