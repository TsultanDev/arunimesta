module;
#include <string>
export module template;

//nama namespace dan sesuai nama file
export namespace template{
    class Template{
        // private di atas dan public dibawah. untuk struct kebalikannya
        // data duluan baru fungsi
        private:
            //Inisialisasi langsung datanya walupun belum ada data
            // format camelCase dengan prefiks s....
            //const void& sFactory();  (untuk yang harus dari factory class)
            const void& sRef(); //inisializasi harus seperti ini. Wajib variable(...).
            int sValue(0);
            std::string sName("MyName");

            // selalu dahulukan konstruktor, sCreate diatas dan sDestroy dibawah, sementara fungsi internal lain berada setelah fungsi destroy
            Template(void& ref) : sRef(ref) {}
            //jika object yang harus dibuat factor maka Template(factory, ref) untuk factory& dan ref& yang pnaggil dari method factory.create(ref).

            //buat create dan destroy jika ingin lebih rapi
            void sCreate(){

            }
            void sDestroy(){

            }
            //fungsi internal disini. setiap pekerjaan dipecah, misalnya fungsi loadModule,  fungsi loadFungsiDinamis
            void sFn1(){

            }
            void sFn2(){

            }

            //buat agar sesama module internal yang hanya boleh menganbil data.
            // dilarang membuat getter.
            friend class A;

        public:
            static std::expected<Template, Error> create(){
                //fungsi create yang harus divalidasi.
                // static untuk class yang tidak ada factory class
                // sementara class yang ada factory class harus melewati
                // factory.createTemplate()
            }
            ~Template(){
                //pembersihan resource
            }
            //delete copy
            Template(const Tempate& other) = delete;
            Template& operator=(const Template&  other) = delete;

            Template(Tempate&& other) noexcept {
                //move value
            };
            Template& operator=(const Template& other) noexcept {
                //move value
            };

            //fungsi publik saja
            void sFnPub1(){

            }
            void sFnPub2(){

            }
            std::expected<Product, Error> createAnother(){

            }

    }
}
